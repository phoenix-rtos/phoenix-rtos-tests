import io
import signal
import socket
from contextlib import contextmanager
from pathlib import Path
from typing import Optional, Union, List, TextIO

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

    def load_program(self, path: Union[Path, str]):
        try:
            self.proc.sendline(f"load {path}")
            self.proc.expect_exact("Start address")
            self.expect_prompt()
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError(f"Failed to load {path}", output=self.logfile.getvalue()) from e

    def cont(self):
        try:
            self.proc.sendline("c")
            self.proc.expect_exact("Continuing.")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError("Failed to continue", output=self.logfile.getvalue()) from e

    def reset(self):
        try:
            self.proc.sendline("monitor reset")
            self.expect_prompt()
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError("Failed to reset", output=self.logfile.getvalue()) from e

    def pause(self):
        try:
            self.proc.sendcontrol("c")
            self.proc.expect_exact("Program received signal SIGINT, Interrupt.")
            self.expect_prompt()
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError("Failed to pause", output=self.logfile.getvalue()) from e

    def set_architecture(self, architecture: str):
        try:
            self.proc.sendline(f"set architecture {architecture}")
            self.proc.expect_exact("The target architecture is set to")
            self.expect_prompt()
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GdbError("Failed to set architecture", output=self.logfile.getvalue()) from e

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


class OpenocdProcess:
    """Handler for Openocd process"""

    def __init__(
        self,
        interface: Optional[str] = None,
        target: Optional[str] = None,
        board: Optional[str] = None,
        extra_args: Optional[List[str]] = None,
        host_log: Optional[TextIO] = None,
        cwd: Optional[Path] = None,
    ):
        self.proc = None
        self._shutdown_done = False
        self.interface = interface
        self.target = target
        self.board = board
        self.extra_args = extra_args
        self.host_log = host_log if host_log is not None else io.StringIO()
        self.cwd = cwd
        self.output = ""

        if board:
            if target or interface:
                raise OpenocdError("Target or Interface arguments provided when board is specified")
        else:
            if not target or not interface:
                raise OpenocdError("Target or Interface arguments missing")

    def _start(self):
        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        if self.board:
            args = ["-f", f"board/{self.board}.cfg"]
        else:
            args = ["-f", f"interface/{self.interface}.cfg", "-f", f"target/{self.target}.cfg"]

        if self.extra_args:
            args.extend(self.extra_args)

        self.proc = pexpect.spawn("openocd", args, encoding="ascii", logfile=self.host_log, cwd=self.cwd)

        try:
            self.proc.expect_exact("Info : Listening on port")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            self.output = self.host_log.getvalue()
            raise OpenocdError("Failed to connect to target", output=self.output) from e

    def _close(self):
        if not self.proc:
            return

        try:
            self.proc.expect(pexpect.EOF, timeout=15)
            self.proc.close()
            self.output = self.host_log.getvalue()
        except pexpect.TIMEOUT:
            clear_pexpect_buffer(self.proc)
            self.proc.close(force=True)
            self.output = self.host_log.getvalue()
            raise OpenocdError(f"Timeout: OpenOCD exited with signal {self.proc.signalstatus}", output=self.output)

        if self.proc.exitstatus != 0:
            raise OpenocdError(f"OpenOCD exited with status {self.proc.exitstatus}", output=self.output)

    def shutdown(self):
        if self._shutdown_done:
            return

        try:
            with socket.create_connection(("localhost", 4444), timeout=2.0) as sock:
                sock.sendall(b"shutdown\n")
        except (socket.timeout, ConnectionRefusedError) as e:
            self.output = self.host_log.getvalue()
            raise OpenocdError("Failed to send shutdown to OpenOCD", output=self.output) from e

        self._shutdown_done = True

    def run(self):
        try:
            self._start()
        finally:
            self._close()


class OpenocdGdbServer(OpenocdProcess):
    """Handler for OpenocdGdbServer process"""

    @contextmanager
    @add_output_to_exception(OpenocdError)
    def run(self):
        try:
            self._start()
            yield self
        finally:
            self._close()

    def _close(self):
        self.shutdown()
        super()._close()


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


class PyocdError(ProcessError):
    name = "PYOCD"


class PyocdProcess:
    """Handler for Pyocd process"""

    def __init__(
        self,
        target: str = None,
        extra_args: Optional[List[str]] = None,
        host_log: Optional[TextIO] = None,
        cwd: Optional[Path] = None,
    ):
        self.target = target
        self.extra_args = extra_args
        self.host_log = host_log if host_log is not None else io.StringIO()
        self.cwd = cwd
        self.proc = None
        self.output = ""

    def _spawn(self, args, end_message: str):
        try:
            if self.extra_args:
                args.extend(self.extra_args)

            self.proc = pexpect.spawn("pyocd", args, encoding="ascii", logfile=self.host_log, cwd=self.cwd)
            self.proc.expect_exact(f"Target type is {self.target}")
            self.proc.expect_exact(end_message, timeout=60)
            self.proc.expect(pexpect.EOF)

        except pexpect.TIMEOUT as e:
            self.output = self.host_log.getvalue()
            raise PyocdError("Failed to connect to target", output=self.output) from e

        finally:
            self._close()

    def _close(self):
        if not self.proc:
            return
        self.proc.close(force=True)
        self.output = self.host_log.getvalue()
        self.host_log.close()

        if self.proc.exitstatus != 0:
            status = self.proc.exitstatus
            if self.proc.exitstatus is None:
                status = "[terminated by signal]"

            raise PyocdError(f"pyocd returned {status} after closing!", output=self.output)

    def load(
        self,
        load_file: str,
        load_offset: int = 0,
    ):
        self.load_file = load_file
        self.load_offset = load_offset
        args = ["load", "--target", self.target, "-v"]

        if self.load_offset != 0:
            args.extend([f"{self.load_file}@{hex(self.load_offset)}"])
        else:
            args.extend([f"{self.load_file}"])

        self._spawn(args=args, end_message="[loader]")

    def reset(self):
        self._spawn(args=["reset", "--target", self.target, "-v"], end_message="Done.")
