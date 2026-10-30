import io
from contextlib import contextmanager
from pathlib import Path
from typing import TextIO

import pexpect

from trunner.dut import clear_pexpect_buffer
from trunner.harness.base import ProcessError


class GrmonError(ProcessError):
    name = "GRMON"


class VibemonError(ProcessError):
    name = "Vibemon"


class GrmonProcess:
    def __init__(
        self,
        extra_args: list[str] | None = None,
        host_log: TextIO | None = None,
        cwd: str = ".",
        jtag_cfg: str | None = None,
    ):
        self.extra_args = extra_args
        self.host_log = host_log if host_log is not None else io.StringIO()
        self.cwd = cwd
        self.jtag_cfg = jtag_cfg
        self.proc = None
        self.output = ""

    def _expect_prompt(self):
        if not self.proc:
            raise GrmonError("grmon process not started")

        try:
            self.proc.expect_exact("grmon4>")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise GrmonError("Failed to read a prompt", output=self.host_log.getvalue()) from e

    def _start(self):
        cmd = "grmon -digilent"
        if self.jtag_cfg:
            cmd += f" -jtagcfg {self.jtag_cfg}"
        self.proc = pexpect.spawn(cmd, encoding="ascii", logfile=self.host_log, cwd=self.cwd)
        self._expect_prompt()

    def _close(self):
        if not self.proc:
            return

        self.proc.sendline("exit")
        try:
            self.proc.expect(pexpect.EOF)
            self.proc.close()
            self.output = self.host_log.getvalue()
        except pexpect.TIMEOUT as e:
            clear_pexpect_buffer(self.proc)
            self.proc.close(force=True)
            self.output = self.host_log.getvalue()
            raise GrmonError("Grmon did not exit gracefully", output=self.host_log.getvalue())

    @contextmanager
    def run(self):
        try:
            self._start()
            yield self
        except GrmonError as e:
            raise e
        finally:
            self._close()

    def load(self, file: str, binary=False, offset=None):
        if not self.proc:
            raise GrmonError("grmon process not started")

        try:
            if binary:
                cmd = f"load -binary {file}"
            else:
                cmd = f"load {file}"

            if offset:
                cmd += f" {offset}"

            self.proc.sendline(cmd)

            match = self.proc.expect_exact([f"Image {self.cwd}/{file} loaded", "Bad argument"], timeout=None)
            if match != 0:
                self.output = self.host_log.getvalue()
                raise GrmonError(f"Failed to load the file: {file}", output=self.output)

        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            self.output = self.host_log.getvalue()
            raise GrmonError(f"Timeout to load the file: {file}", output=self.output)

    def detach(self):
        if not self.proc:
            raise GrmonError("grmon process not started")

        self.proc.sendline("detach")
        self._expect_prompt()


class VibemonProcess:
    def __init__(
        self,
        extra_args: list[str] | None = None,
        host_log: TextIO | None = None,
        cwd: Path | str = ".",
    ):
        self.extra_args = extra_args
        self.host_log = host_log if host_log is not None else io.StringIO()
        self.cwd = cwd
        self.proc = None
        self.output = ""

    def _expect_prompt(self):
        if not self.proc:
            raise VibemonError("vibemon process not started")

        try:
            self.proc.expect(r"vibemon\[[^\]]+\]>\s*")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise VibemonError("Failed to read a prompt", output=self.host_log.getvalue()) from e

    def _start(self):
        self.proc = pexpect.spawn("vibemon", encoding="ascii", logfile=self.host_log, cwd=self.cwd)
        self._expect_prompt()

    def _close(self):
        if not self.proc:
            return

        self.proc.sendline("exit")
        try:
            self.proc.expect(pexpect.EOF)
            self.proc.close()
            self.output = self.host_log.getvalue()
        except pexpect.TIMEOUT as e:
            clear_pexpect_buffer(self.proc)
            self.proc.close(force=True)
            self.output = self.host_log.getvalue()
            raise VibemonError("Vibemon did not exit gracefully", output=self.host_log.getvalue())

    @contextmanager
    def run(self):
        try:
            self._start()
            yield self
        except VibemonError as e:
            raise e
        finally:
            self._close()

    def load(self, file: str, binary=False, offset=None):
        if not self.proc:
            raise VibemonError("vibemon process not started")

        try:
            if binary:
                cmd = f"load -binary {file}"
            else:
                cmd = f"load {file}"

            if offset:
                cmd += f" {offset}"

            self.proc.sendline(cmd)

            match = self.proc.expect([r"load:\s*\d+", "load: No such file"], timeout=None)
            if match != 0:
                self.output = self.host_log.getvalue()
                raise VibemonError(f"Failed to load the file: {file}", output=self.output)

        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            self.output = self.host_log.getvalue()
            raise VibemonError(f"Timeout to load the file: {file}", output=self.output)

    def run_and_resume(self):
        if not self.proc:
            raise VibemonError("vibemon process not started")

        self.proc.sendline("run")
        self.proc.expect_exact("started")
        self.proc.send("\003")
        self.proc.sendline("resume")
        self._expect_prompt()
