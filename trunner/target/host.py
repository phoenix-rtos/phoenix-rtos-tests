import shlex
from typing import Callable, Optional, List

from trunner.config import TestContext
from trunner.dut import HostDut
from trunner.harness import HarnessBase, HarnessBuilder
from trunner.types import TestOptions, TestResult
from .base import TargetBase


class HostPCGenericTarget(TargetBase):
    name = "host-generic-pc"
    rootfs = True
    sysexec = False
    experimental = True

    class ExecHarness(HarnessBase):
        def __init__(self, dut: HostDut, cmd: List[str]):
            self.dut = dut
            self.cmd = " ".join(shlex.quote(arg) for arg in cmd)
            super().__init__()

        def __call__(self) -> Optional[TestResult]:
            self.dut.set_args(self.cmd, encoding="utf-8")
            self.dut.open()
            return self.harness()

    def __init__(self):
        self.dut = HostDut()
        super().__init__()

    @classmethod
    def from_context(cls, ctx: TestContext):
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

            builder.chain(fail)
            return builder.get_harness()

        builder.chain(self.ExecHarness(self.dut, test.shell.cmd))
        builder.chain(test.harness)

        return builder.get_harness()
