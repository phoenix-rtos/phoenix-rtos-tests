import io
import os
import signal
import threading
import time
import pexpect

from trunner.harness import ProcessError
from serial.tools import list_ports
from contextlib import contextmanager
from .common import add_output_to_exception


def wait_for_vid_pid(vid: int, pid: int, timeout=0):
    """wait for connected usb serial device with required vendor & product id"""

    asleep = 0
    found_ports = []

    while not found_ports:
        time.sleep(0.01)
        asleep += 0.01

        found_ports = [port for port in list_ports.comports() if port.pid == pid and port.vid == vid]

        if len(found_ports) > 1:
            raise Exception(
                "More than one plo port was found! Maybe more than one device is connected? Hint used to find port:"
                f"{vid:04x}:{pid:04x}"
            )

        if timeout and asleep >= timeout:
            raise TimeoutError(f"Couldn't find plo USB device with vid/pid: '{vid:04x}:{pid:04x}'")

    return found_ports[0].device


class PsuError(ProcessError):
    name = "PSU"


class Psu:
    def __init__(self, script, cwd=None):
        self.script = script
        self.cwd = cwd
        self.proc = None

    def read_output(self):
        if not self.proc:
            return

        while True:
            line = self.proc.readline()
            if not line:
                break

    def run(self):
        # TODO handle lack of psu program?
        self.proc = pexpect.spawn(
            "psu",
            [self.script],
            cwd=self.cwd,
            encoding="ascii",
        )

        try:
            self.proc.expect(pexpect.EOF, timeout=20)
        except pexpect.TIMEOUT as e:
            raise PsuError(f"Failed to execute {self.script} plo script (timeout)", output=self.proc.before) from e

        self.proc.wait()

        if self.proc.exitstatus != 0:
            raise PsuError(
                f"Loading plo using psu failed, psu return code {self.proc.exitstatus}", output=self.proc.before
            )


class PhoenixdError(ProcessError):
    name = "PHOENIXD"


class Phoenixd:
    """Handler for phoenixd process"""

    def __init__(self, vid=0x16F9, pid=0x0003, cwd=".", directory="."):
        self.vid = vid
        self.pid = pid
        self.proc = None
        self.reader_thread = None
        self.cwd = cwd
        self.dir = directory
        self.dispatcher_event = None
        self.logfile = io.StringIO()
        self.output = ""

    def _reader(self):
        """This method is intended to be run as a separate thread.
        It searches for a line stating that message dispatcher has started
        and then browses phoenixd output that may be needed for proper working"""

        msg = f"Starting message dispatcher on [{self.port}]"

        try:
            self.proc.expect_exact(msg, timeout=4)
        # The main thread will catch that dispatcher didn't start
        except (pexpect.EOF, pexpect.TIMEOUT):
            return

        self.dispatcher_event.set()

        # Phoenixd requires reading its output constantly to work properly, especially during long copy operations
        # EOF occurs always after killing the phoenixd process
        self.proc.expect(pexpect.EOF, timeout=None)

    def _run(self):
        try:
            self.port = wait_for_vid_pid(self.vid, self.pid, timeout=10)
        except (TimeoutError, Exception) as exc:
            raise PhoenixdError(str(exc)) from exc

        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            "phoenixd",
            ["-p", self.port, "-s", str(self.dir)],
            cwd=self.cwd,
            encoding="ascii",
            logfile=self.logfile,
        )

        self.dispatcher_event = threading.Event()
        self.reader_thread = threading.Thread(target=self._reader)
        self.reader_thread.start()

        # Reader thread will notify us that message dispatcher has just started
        dispatcher_ready = self.dispatcher_event.wait(timeout=5)
        if not dispatcher_ready:
            raise PhoenixdError("Message dispatcher did not start!", self.proc.before)

        return self.proc

    @contextmanager
    @add_output_to_exception(PhoenixdError)
    def run(self):
        try:
            self._run()
            yield
        finally:
            self._close()

    def _close(self):
        if not self.proc:
            return

        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)
        self.reader_thread.join(timeout=10)
        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)

        self.output = self.logfile.getvalue()
        self.logfile.close()
