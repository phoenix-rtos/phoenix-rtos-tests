import re
import shlex
from typing import Optional, List

import pexpect

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


class ShellHarness(IntermediateHarness):
    """Basic harness for the shell.

    It waits for the prompt and then executes the command if given.

    Attributes:
        dut: Device on which harness will be run.
        prompt: Prompt that shell outputs.
        cmd: Command that will be executed after reading prompt.
        prompt_timeout: Optional timeout to wait before prompt will show up.

    """

    def __init__(
        self,
        dut: Dut,
        prompt: str,
        cmd: Optional[List[str]] = None,
        prompt_timeout: int = -1,
        suppress_dmesg: bool = True,
    ):
        super().__init__()
        self.dut = dut
        self.prompt = prompt
        self.cmd = " ".join(map(shlex.quote, cmd)) if cmd is not None else cmd
        self.prompt_timeout = prompt_timeout
        self.suppress_dmesg = suppress_dmesg

    def assert_prompt(self):
        try:
            self.dut.expect_exact(self.prompt, timeout=self.prompt_timeout)
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise ShellError(
                msg="Couldn't find a prompt!",
                expected=self.prompt,
                output=self.dut.before,
            ) from e

    def assert_cmd_succeded(self):
        self.dut.sendline("echo $?")

        EOL = r"(?:\r+)\n"
        expected = rf"(\d+?){EOL}"

        try:
            self.dut.expect(expected, timeout=5)
            exit_code = int(self.dut.match.group(1))
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise ShellError(
                msg="Couldn't get exit status!",
                expected=self.prompt,
                output=self.dut.before,
            ) from e

        if exit_code != 0:
            raise ShellError(
                msg="Command returned a non-zero status"
            )

        self.assert_prompt()

    def assert_test_succeded(self):
        try:
            self.assert_prompt()
        except ShellError:
            # If the prompt was lost, check the system's responsiveness
            self.dut.send("\n")
            self.assert_prompt()

        if self.cmd:
            self.assert_cmd_succeded()

    def __call__(self, result: TestResult) -> TestResult:
        self.assert_prompt()

        # suppress klog output to console while test is running to avoid problems with parsing
        if self.suppress_dmesg:
            self.dut.pexpect_proc.sendline("dmesg -D")
            self.assert_prompt()

        if self.cmd is not None:
            self.dut.sendline(self.cmd)
            try:
                self.dut.expect(f"{re.escape(self.cmd)}(\r+)\n")
            except (pexpect.TIMEOUT, pexpect.EOF) as e:
                raise ShellError(
                    msg="Couldn't find the echoed command!",
                    expected=self.cmd,
                    output=self.dut.before,
                ) from e

        result.set_stage(TestStage.RUN)
        test_result = self.next_harness(result)

        if test_result.status is not Status.FAIL:
            self.assert_test_succeded()

            # re-enable log output to release collected output
            if self.suppress_dmesg:
                self.dut.pexpect_proc.sendline("dmesg -E")
                self.assert_prompt()

        return test_result
