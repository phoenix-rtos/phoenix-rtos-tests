import shlex
from typing import Optional, List

import pexpect

from trunner.dut import Dut
from trunner.text import bold
from trunner.types import TestResult
from .base import HarnessBase, HarnessError


class ShellError(HarnessError):
    def __init__(
        self,
        msg: Optional[str] = None,
        output: Optional[str] = None,
        expected: Optional[str] = None,
        cmd: Optional[str] = None,
    ):
        self.msg = msg
        self.output = output
        self.cmd = cmd
        self.expected = expected
        super().__init__(self)

    def __str__(self):
        err = bold("SHELL ERROR: ") + (self.msg if self.msg else "") + "\n"
        if self.cmd is not None:
            err += bold("CMD PASSED: ") + self.cmd + "\n"

        if self.expected is not None:
            err += bold("EXPECTED: ") + self.expected + "\n"

        if self.output is not None:
            err += bold("OUTPUT:") + "\n" + self.output + "\n"

        return err


class ShellHarness(HarnessBase):
    """Basic harness for the shell.

    It waits for the prompt and then executes the command if given.

    Attributes:
        dut: Device on which harness will be run.
        prompt: Prompt that shell outputs.
        cmd: Command that will be executed after reading prompt.
        prompt_timeout: Optional timeout to wait before prompt will show up.

    """

    def __init__(self, dut: Dut, prompt: str, cmd: Optional[List[str]] = None, prompt_timeout: Optional[int] = None):
        self.dut = dut
        self.prompt = prompt
        if cmd is None:
            self.cmd = cmd
        else:
            self.cmd = " ".join(shlex.quote(arg) for arg in cmd)
        self.prompt_timeout = prompt_timeout
        super().__init__()

    def __call__(self) -> Optional[TestResult]:
        if self.prompt_timeout is not None:
            timeout = self.prompt_timeout
        else:
            timeout = self.dut.timeout

        try:
            self.dut.expect_exact(self.prompt, timeout=timeout)
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

        return self.harness()
