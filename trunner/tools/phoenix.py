import io
import os
import signal
import threading
import time
from contextlib import contextmanager

import pexpect

from trunner.harness import ProcessError, HarnessError
from .common import add_output_to_exception


def wait_for_dev(port, timeout=0):
    asleep = 0

    # naive wait for dev
    while not os.path.exists(port):
        time.sleep(0.01)
        asleep += 0.01
        if timeout and asleep >= timeout:
            raise TimeoutError


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

    def __init__(self, port="/dev/serial/by-id/usb-Phoenix_Systems_plo_CDC_ACM-if00", cwd=".", directory="."):
        self.port = port
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
            wait_for_dev(self.port, timeout=10)
        except TimeoutError as exc:
            raise PhoenixdError(f"couldn't find {self.port}") from exc

        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            "phoenixd",
            ["-p", self.port, "-s", self.dir],
            cwd=self.cwd,
            encoding="ascii",
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
    @add_output_to_exception(exclude=PhoenixdError)
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
