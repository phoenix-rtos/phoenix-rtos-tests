import io
import signal
from contextlib import contextmanager
from pathlib import Path
from typing import Optional, Union, List

import pexpect

from trunner.harness import ProcessError
from trunner.dut import clear_pexpect_buffer
from .common import add_output_to_exception


class GdbError(ProcessError):
    name = "GDB"


class GdbInteractive:
    def __init__(self, port: int = 3333, cwd: Union[Path, str] = "."):
        self.port = port
        self.cwd = cwd
        self.proc = None
        self.output = ""
        self.logfile = io.StringIO()

    def expect_prompt(self):
        try:
            self.proc.expect_exact("(gdb) ")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError("Failed to read a prompt", output=self.logfile.getvalue()) from e

    def connect(self):
        try:
            self.proc.sendline(f"target remote localhost: {self.port}")
            # TODO here is not enough to expect a prompt, parse if everything is OK
            self.expect_prompt()
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError(f"Failed to connect to localhost:{self.port}", output=self.logfile.getvalue()) from e

    def load(self, test_path: Union[Path, str], addr: int):
        try:
            self.proc.sendline(f"restore {test_path} binary {addr}")
            self.proc.expect_exact("Restoring binary file")
            self.expect_prompt()
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError(f"Failed to load {test_path} to addr {addr}", output=self.logfile.getvalue()) from e

    def cont(self):
        try:
            self.proc.sendline("c")
            self.proc.expect_exact("Continuing.")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError("Failed to continue", output=self.logfile.getvalue()) from e

    def _close(self):
        if not self.proc:
            return

        self.proc.close(force=True)
        self.proc.wait()
        self.output = self.logfile.getvalue()
        self.logfile.close()

        if self.proc.exitstatus != 0:
            status = self.proc.exitstatus
            if self.proc.exitstatus is None:
                status = "[terminated by signal]"

            raise GdbError(
                f"gdb-multiarch returned {status} after closing gdb in interactive mode!", output=self.output
            )

    @contextmanager
    @add_output_to_exception(GdbError)
    def run(self):
        try:
            self.proc = pexpect.spawn(
                "gdb-multiarch",
                encoding="ascii",
                timeout=8,
                cwd=str(self.cwd),
                logfile=self.logfile,
            )
            self.proc.expect_exact("(gdb) ")
            yield
        finally:
            self._close()


class OpenocdError(ProcessError):
    name = "OPENOCD"


class OpenocdGdbServer:
    """Handler for OpenocdGdbServer process"""

    def __init__(
        self,
        interface: Optional[str] = None,
        target: Optional[str] = None,
        board: Optional[str] = None,
        extra_args: Optional[List[str]] = None,
    ):
        self.proc = None
        self.target = target
        self.interface = interface
        self.board = board
        self.output = ""
        self.logfile = io.StringIO()
        self.extra_args = extra_args

        if board:
            if target or interface:
                raise OpenocdError("Target or Interface arguments provided when board is specified")
        else:
            if not target or not interface:
                raise OpenocdError("Target or Interface arguments missing")

    @contextmanager
    @add_output_to_exception(OpenocdError)
    def run(self):
        try:
            # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
            if self.board:
                args = ["-f", f"board/{self.board}.cfg"]
            else:
                args = ["-f", f"interface/{self.interface}.cfg", "-f", f"target/{self.target}.cfg"]

            if self.extra_args:
                args.extend(self.extra_args)

            self.proc = pexpect.spawn("openocd", args, encoding="ascii", logfile=self.logfile)

            try:
                self.proc.expect_exact("Info : Listening on port 3333 for gdb connections")
            except (pexpect.TIMEOUT, pexpect.EOF) as e:
                raise OpenocdError("Failed to start gdb server", self.logfile.getvalue()) from e

            yield
        finally:
            self._close()

    def _close(self):
        if not self.proc:
            return

        clear_pexpect_buffer(self.proc)
        self.proc.close(force=True)
        self.proc.wait()
        self.output = self.logfile.getvalue()
        self.logfile.close()


class JLinkGdbServer:
    def __init__(self, device):
        self.device = device
        self.proc = None
        self.logfile = io.StringIO()
        self.output = ""

    def _close(self):
        if not self.proc:
            return

        self.proc.kill(signal.SIGTERM)
        self.proc.expect_exact("Shutting down...")
        self.proc.close(force=True)
        self.output = self.logfile.getvalue()
        self.logfile.close()

    @contextmanager
    @add_output_to_exception()
    def run(self):
        try:
            self.proc = pexpect.spawn(
                "JLinkGDBServer", ["-device", self.device], encoding="ascii", logfile=self.logfile
            )
            self.proc.expect_exact("Connected to target")
            self.proc.expect_exact("Waiting for GDB connection...")
            yield
        finally:
            self._close()
