import shlex
from typing import Callable, List

from trunner.ctx import TestContext
from trunner.dut import HostDut
from trunner.harness import IntermediateHarness, HarnessBuilder
from trunner.types import TestOptions, TestResult, TestStage
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

        def __call__(self, result: TestResult) -> TestResult:
            result.set_stage(TestStage.FLASH)
            self.dut.set_args(self.cmd, encoding="utf-8")
            self.dut.open()
            result.set_stage(TestStage.RUN)
            res = self.next_harness(result)

            # ensure the process exited with status = 0 and we've processed all of the output
            try:
                exitstatus = self.dut.wait()
                if exitstatus != 0:
                    result.fail(msg=f"non-zero exit status: {exitstatus}", summary="Exit Status failure")
            except TimeoutError as exc:
                result.fail_harness_exception(exc)

            return res

    def __init__(self):
        super().__init__()
        self.dut: HostDut = HostDut()

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()

    def flash_dut(self):
        pass

    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        builder = HarnessBuilder()

        if test.shell is None or test.shell.cmd is None:
            # TODO we should detect it in parsing step, now force fail
            def fail(result: TestResult):
                result.fail(msg="There is no command to execute")
                return result

            builder.add(fail)
            return builder.get_harness()

        test.shell.cmd[0] = f"{self.root_dir()}{test.shell.cmd[0]}"
        builder.add(self.ExecHarness(self.dut, test.shell.cmd))
        builder.add(test.harness)

        return builder.get_harness()
