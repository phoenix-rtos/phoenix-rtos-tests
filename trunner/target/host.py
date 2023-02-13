import shlex
from typing import Callable, Optional, List

from trunner.config import TestContext
from trunner.dut import HostDut
from trunner.harness import IntermediateHarness, HarnessBuilder
from trunner.types import TestOptions, TestResult
from .base import TargetBase


class HostPCGenericTarget(TargetBase):
    name = "host-generic-pc"
    rootfs = True
    sysexec = False
    experimental = True

    class ExecHarness(IntermediateHarness):
        def __init__(self, dut: HostDut, cmd: List[str]):
            super().__init__()
            self.dut = dut
            self.cmd = " ".join(map(shlex.quote, cmd))

        def __call__(self) -> Optional[TestResult]:
            self.dut.set_args(self.cmd, encoding="utf-8")
            self.dut.open()
            return self.next_harness()

    def __init__(self):
        super().__init__()
        self.dut = HostDut()

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()

    def exec_dir(self) -> str:
        return self.bin_dir()

    def flash_dut(self):
        pass

    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        builder = HarnessBuilder()

        if test.shell is None or test.shell.cmd is None:
            # TODO we should detect it in parsing step, now force fail
            def fail():
                return TestResult(msg="There is no command to execute", status=TestResult.FAIL)

            builder.add(fail)
            return builder.get_harness()

        builder.add(self.ExecHarness(self.dut, test.shell.cmd))
        builder.add(test.harness)

        return builder.get_harness()
