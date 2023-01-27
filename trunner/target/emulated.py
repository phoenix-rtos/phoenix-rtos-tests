from typing import Callable, Optional

from trunner.config import TestContext
from trunner.dut import QemuDut
from trunner.harness import HarnessBuilder, RebooterHarness, ShellHarness
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

    def flash_dut(self):
        pass

    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.chain(RebooterHarness(self.rebooter))

        if test.shell is not None:
            builder.chain(ShellHarness(self.dut, self.shell_prompt, test.shell.cmd))

        builder.chain(test.harness)

        return builder.get_harness()


class IA32GenericQemuTarget(QemuTarget):
    name = "ia32-generic-qemu"
    rootfs = True

    def __init__(self):
        super().__init__("ia32-generic-qemu-test.sh")

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls()


class RISCV64GenericQemuTarget(QemuTarget):
    name = "riscv64-generic-qemu"
    rootfs = False
    experimental = True

    def __init__(self):
        super().__init__("riscv64-generic-qemu.sh")

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls()


class ARMv7A9Zynq7000QemuTarget(QemuTarget):
    name = "armv7a9-zynq7000-qemu"
    rootfs = True
    shell_prompt = "root@?:~ # "

    def __init__(self):
        super().__init__("armv7a9-zynq7000-qemu.sh")

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls()

    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        harness = super().build_test(test)

        if test.shell is not None:
            # Start of the zynq target take around 45 seconds due to the slow filesystem initialization.
            # Iterate over harness chain to find a ShellHarness to increase prompt_timeout value.

            node = harness
            while node:
                if isinstance(node, ShellHarness):
                    node.prompt_timeout = 60

                node = getattr(node, "harness", None)

        return harness
