import shlex
from typing import Optional, List

import pexpect

from trunner.dut import Dut
from trunner.text import bold
from trunner.types import TestResult
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

    def __init__(self, dut: Dut, prompt: str, cmd: Optional[List[str]] = None, prompt_timeout: Optional[int] = -1):
        super().__init__()
        self.dut = dut
        self.prompt = prompt
        self.cmd = " ".join(map(shlex.quote, cmd)) if cmd is not None else cmd
        self.prompt_timeout = prompt_timeout

    def __call__(self) -> Optional[TestResult]:
        try:
            self.dut.expect_exact(self.prompt, timeout=self.prompt_timeout)
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise ShellError(
                msg="Couldn't find a prompt!",
                expected=self.prompt,
                output=self.dut.before,
            ) from e

        if self.cmd is not None:
            self.dut.send(self.cmd + "\n")
            try:
                self.dut.expect(f"{self.cmd}(\r+)\n")
            except (pexpect.TIMEOUT, pexpect.EOF) as e:
                raise ShellError(
                    msg="Couldn't find the echoed command!",
                    expected=self.prompt,
                    output=self.dut.before,
                ) from e

        return self.next_harness()
