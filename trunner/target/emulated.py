from abc import abstractmethod
from typing import Callable

from trunner.ctx import TestContext
from trunner.dut import QemuDut
from trunner.harness import HarnessBuilder, RebooterHarness, ShellHarness, TestStartRunningHarness
from trunner.types import TestOptions, TestResult
from .base import TargetBase


class QemuDutRebooter:
    def __init__(self, dut: QemuDut):
        self.dut = dut

    def __call__(self, flash=False, hard=False):
        self.dut.close()
        self.dut.open()


class QemuTarget(TargetBase):
    def __init__(self, script: str):
        super().__init__()
        self.script = script
        # TODO Make sure that script path exists
        self.dut = QemuDut(f"{self.project_dir}/scripts/{self.script}", encoding="utf-8")
        self.rebooter = QemuDutRebooter(self.dut)

    @classmethod
    @abstractmethod
    def from_context(cls, _: TestContext):
        pass

    def flash_dut(self):
        pass

    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter))

        if test.shell is not None:
            builder.add(
                ShellHarness(
                    self.dut,
                    self.shell_prompt,
                    test.shell.cmd,
                    prompt_timeout=self.prompt_timeout,
                )
            )
        else:
            builder.add(TestStartRunningHarness())

        builder.add(test.harness)

        return builder.get_harness()


class IA32GenericQemuTarget(QemuTarget):
    name = "ia32-generic-qemu"
    rootfs = True

    def __init__(self):
        super().__init__("ia32-generic-qemu-test.sh")

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()


class RISCV64GenericQemuTarget(QemuTarget):
    name = "riscv64-generic-qemu"
    rootfs = True
    experimental = True

    def __init__(self):
        super().__init__("riscv64-generic-qemu.sh")

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()


class ARMv7A9Zynq7000QemuTarget(QemuTarget):
    name = "armv7a9-zynq7000-qemu"
    rootfs = True
    shell_prompt = "root@?:~ # "

    def __init__(self):
        super().__init__("armv7a9-zynq7000-qemu.sh")
        # Start of the zynq target take around 45 seconds due to the slow filesystem initialization.
        # Iterate over harness chain to find a ShellHarness to increase prompt_timeout value.
        self.prompt_timeout = 60

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()
