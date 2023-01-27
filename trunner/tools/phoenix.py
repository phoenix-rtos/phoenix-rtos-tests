import os
import signal
import threading
import time
from contextlib import contextmanager

import pexpect

from trunner.harness.base import HarnessError
from trunner.text import bold


def wait_for_dev(port, timeout=0):
    asleep = 0

    # naive wait for dev
    while not os.path.exists(port):
        time.sleep(0.01)
        asleep += 0.01
        if timeout and asleep >= timeout:
            raise TimeoutError


class PsuError(HarnessError):
    def __init__(self, msg, output=None):
        self.msg = msg
        self.output = output

    def __str__(self):
        err = bold("PSU ERROR:") + "\n" + (self.msg if self.msg else "") + "\n"
        if self.output is not None:
            err += bold("OUTPUT:") + "\n" + self.output + "\n"

        return err


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


class PhoenixdError(HarnessError):
    def __init__(self, msg, output=None):
        self.msg = msg
        self.output = output

    def __str__(self):
        err = bold("PHOENIXD ERROR:") + "\n" + (self.msg if self.msg else "") + "\n"
        if self.output is not None:
            err += bold("OUTPUT:") + "\n" + self.output + "\n"

        return err


class Phoenixd:
    """Handler for phoenixd process"""

    def __init__(self, port=None, cwd=".", directory="."):
        if port is None:
            self.port = "/dev/serial/by-id/usb-Phoenix_Systems_plo_CDC_ACM-if00"
        else:
            self.port = port

        self.proc = None
        self.output_buffer = ""
        self.reader_thread = None
        self.cwd = cwd
        self.dir = directory
        self.dispatcher_event = None

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

        self.output_buffer = self.proc.before
        # Phoenixd requires reading its output constantly to work properly, especially during long copy operations
        self.proc.expect(pexpect.EOF, timeout=None)
        # EOF occurs always after killing the phoenixd process
        self.output_buffer += self.proc.before

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
            self.kill()
            raise PhoenixdError("Message dispatcher did not start!", self.output_buffer)

        return self.proc

    @contextmanager
    def run(self):
        try:
            self._run()
            yield
        finally:
            self.kill()

    def output(self):
        # if is_github_actions():
        #    output = '::group::phoenixd output\n' + self.output_buffer + '\n::endgroup::\n'
        # else:
        output = self.output_buffer
        return output

    def close(self):
        self.kill()

    def kill(self):
        if not self.proc:
            return

        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)
        self.reader_thread.join(timeout=10)
        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)

    def __enter__(self):
        self._run()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.kill()
