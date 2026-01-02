import time
import re
import pexpect
import shlex

from typing import Optional

from trunner.dut import Dut
from trunner.text import bold
from trunner.types import TestResult, TestStage, Status
from .base import HarnessError, IntermediateHarness


class ShellError(HarnessError):
    def __init__(
        self,
        msg: str = "",
        output: Optional[str] = None,
        expected: Optional[str] = None,
        cmd: Optional[str] = None,
    ):
        super().__init__()
        self.msg = msg
        self.output = output
        self.cmd = cmd
        self.expected = expected

    def __str__(self):
        err = [bold("SHELL ERROR: ") + self.msg]

        if self.cmd is not None:
            err.append(bold("CMD PASSED: ") + self.cmd)

        if self.expected is not None:
            err.append(bold("EXPECTED: ") + self.expected)

        if self.output is not None:
            err.extend([bold("OUTPUT:"), self.output])

        err.append("")
        return "\n".join(err)


class Shell():
    """Harness providing basic shell interaction."""

    def __init__(
        self,
        dut: Dut,
        prompt: str
    ):
        super().__init__()
        self.dut = dut
        self.prompt = prompt

    def assert_prompt(self, timeout: int = 30):
        try:
            self.dut.expect_exact(self.prompt, timeout=timeout)
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise ShellError(
                msg="Couldn't find a prompt!",
                expected=self.prompt,
                output=self.dut.before
            ) from e

        # Wait for the shell to take control of the terminal
        time.sleep(0.1)

    def send_cmd(self, cmd: str):
        self.dut.sendline(cmd)
        try:
            self.dut.expect(f"{re.escape(cmd)}(\r+)\n")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise ShellError(
                msg="Couldn't find the echoed command!",
                expected=cmd,
                output=self.dut.before
            ) from e


class ShellHarness(IntermediateHarness):
    """Harness used only to assert the system entered the shell."""

    def __init__(self, shell: Shell, boot_timeout: int = 30):
        super().__init__()
        self.shell = shell
        self.boot_timeout = boot_timeout

    def __call__(self, result: TestResult) -> TestResult:
        self.shell.assert_prompt(self.boot_timeout)
        return self.next_harness(result)


class TestHarness(IntermediateHarness):
    """Harness to execute the test."""

    def __init__(self, shell: Shell, test_cmd: str, suppress_dmesg: bool = True):
        super().__init__()
        self.shell = shell
        self.test_cmd = " ".join(map(shlex.quote, test_cmd))
        self.suppress_dmesg = suppress_dmesg

    def __call__(self, result: TestResult) -> TestResult:
        # suppress klog output to console while test is running to avoid problems with parsing
        if self.suppress_dmesg:
            self.shell.send_cmd("dmesg -D")
            self.shell.assert_prompt()

        self.shell.send_cmd(self.test_cmd)

        result.set_stage(TestStage.RUN)
        test_result = self.next_harness(result)

        if test_result.status is not Status.FAIL:
            self.shell.assert_prompt()

            # re-enable log output to release collected output
            if self.suppress_dmesg:
                self.shell.send_cmd("dmesg -E")
                self.shell.assert_prompt()

        return test_result
