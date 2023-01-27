import signal
from contextlib import contextmanager
from pathlib import Path
from typing import Union

import pexpect

from trunner.text import bold
from trunner.harness import HarnessError, FlashError


class GdbError(HarnessError):
    def __init__(self, msg=None, output=None):
        self.msg = msg
        self.output = output

    def __str__(self):
        err = bold("GDB ERROR: ") + (self.msg if self.msg else "") + "\n"
        if self.output is not None:
            err += bold("OUTPUT:") + "\n" + self.output + "\n"

        return err


class GdbInteractive:
    def __init__(self, port: int = 3333, cwd: Union[Path, str] = "."):
        self.port = port
        self.cwd = cwd
        self.proc = None

    def expect_prompt(self):
        try:
            self.proc.expect_exact("(gdb) ")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError("Failed to read a prompt", self.proc.before) from e

    def connect(self):
        try:
            self.proc.sendline(f"target remote localhost: {self.port}")
            # TODO here is not enough to expect a prompt, parse if everything is OK
            self.expect_prompt()
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError(f"Failed to connect to localhost:{self.port}", output=self.proc.before) from e

    def load(self, test_path: Union[Path, str], addr: int):
        try:
            self.proc.sendline(f"restore {test_path} binary {addr}")
            self.proc.expect_exact("Restoring binary file")
            self.expect_prompt()
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError(f"Failed to load {test_path} to addr {addr}", output=self.proc.before) from e

    def cont(self):
        try:
            self.proc.sendline("c")
            self.proc.expect_exact("Continuing.")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError("Failed to continue", output=self.proc.before) from e

    def close(self):
        if not self.proc:
            return

        self.proc.close(force=True)
        self.proc.wait()

        if self.proc.exitstatus != 0:
            status = self.proc.exitstatus
            if self.proc.exitstatus is None:
                status = "[terminated by signal]"

            raise GdbError(f"gdb-multiarch returned {status} after closing gdb in interactive mode!")

    @contextmanager
    def run(self):
        try:
            self.proc = pexpect.spawn(
                "gdb-multiarch",
                encoding="ascii",
                timeout=8,
                cwd=self.cwd,
            )
            self.proc.expect_exact("(gdb) ")
            yield
        except FlashError as e:
            # Add the output of GdbInteractive to exception
            raise e
        finally:
            self.close()


class OpenocdError(HarnessError):
    def __init__(self, msg=None, output=None):
        self.msg = msg
        self.output = output

    def __str__(self):
        err = bold("OPENOCD ERROR: ") + (self.msg if self.msg else "") + "\n"
        if self.output is not None:
            err += bold("OUTPUT:") + "\n" + self.output + "\n"

        return err


class OpenocdGdbServer:
    """Handler for OpenocdGdbServer process"""

    def __init__(self, interface: str, target: str):
        self.proc = None
        self.target = target
        self.interface = interface

    @contextmanager
    def run(self):
        try:
            # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
            self.proc = pexpect.spawn(
                "openocd",
                [
                    "-f",
                    f"interface/{self.interface}.cfg",
                    "-f",
                    f"target/{self.target}.cfg",
                    "-c",
                    "reset_config srst_only srst_nogate connect_assert_srst",
                    "-c",
                    "init;reset",
                ],
                encoding="ascii",
            )

            try:
                self.proc.expect_exact("Info : Listening on port 3333 for gdb connections")
            except (pexpect.TIMEOUT, pexpect.EOF) as e:
                raise OpenocdError("Failed to start gdb server", self.proc.before) from e

            yield
        except FlashError as e:
            # Add the output of OpenocdGdbServer to exception
            raise e
        finally:
            self.close()

    def close(self):
        if not self.proc:
            return

        self.proc.close(force=True)
        self.proc.wait()


class JLinkGdbServer:
    def __init__(self, device):
        self.device = device
        self.proc = None

    def close(self):
        if not self.proc:
            return

        self.proc.kill(signal.SIGTERM)
        self.proc.expect_exact("Shutting down...")
        self.proc.close(force=True)

    @contextmanager
    def run(self):
        try:
            self.proc = pexpect.spawn("JLinkGDBServer", ["-device", self.device], encoding="ascii")
            self.proc.expect_exact("Connected to target")
            self.proc.expect_exact("Waiting for GDB connection...")
            yield
        except FlashError as e:
            # Add the output of JLinkGdbServer to exception
            raise e
        finally:
            self.close()
