from abc import abstractmethod
from pathlib import Path
from typing import Callable, Sequence, TextIO

from trunner.ctx import TestContext
from trunner.dut import Dut, QemuDut
from trunner.harness import (
    HarnessBuilder,
    PloHarness,
    PloInterface,
    RebooterHarness,
    ShellHarness,
    TerminalHarness,
    TestStartRunningHarness,
)
from trunner.tools import GdbInteractive
from trunner.types import AppOptions, TestOptions, TestResult
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

    def flash_dut(self, host_log: TextIO):
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

    def __init__(self):
        super().__init__("riscv64-generic-qemu.sh")

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()


class SPARCV8LeonGenericQemuTarget(QemuTarget):
    name = "sparcv8leon-generic-qemu"
    rootfs = False
    experimental = True

    def __init__(self):
        super().__init__("sparcv8leon-generic-qemu-test.sh")

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()


class ARMv7A9Zynq7000QemuTarget(QemuTarget):
    name = "armv7a9-zynq7000-qemu"
    rootfs = True
    shell_prompt = "root@?:~ # "

    def __init__(self):
        super().__init__("armv7a9-zynq7000-qemu-test.sh")
        # Start of the zynq target take around 45 seconds due to the slow filesystem initialization.
        # Iterate over harness chain to find a ShellHarness to increase prompt_timeout value.
        self.prompt_timeout = 60

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()


class AARCH64A53ZynqmpQemuTarget(QemuTarget):
    name = "aarch64a53-zynqmp-qemu"
    rootfs = True

    def __init__(self):
        super().__init__("aarch64a53-zynqmp-qemu.sh")
        # System initialization may take 30+ seconds on this target due to emulation limitations
        self.prompt_timeout = 60

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()


class ARMV7R5FPloAppLoader(TerminalHarness, PloInterface):
    def __init__(self, dut: Dut, apps: Sequence[AppOptions], gdb: GdbInteractive):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.apps = apps
        self.gdb = gdb
        self.ramdisk_addr = 0x8000000
        self.page_sz = 0x200

    def _aligned_app_size(self, path: Path):
        sz = path.stat().st_size
        offset = (self.page_sz - sz) % self.page_sz
        return sz + offset

    def __call__(self):
        with self.gdb.run():
            offset = 0
            self.gdb.connect()

            for app in self.apps:
                path = self.gdb.cwd / Path(app.file)
                sz = self._aligned_app_size(path)
                self.gdb.load(path, self.ramdisk_addr + offset)
                offset += sz

        offset = 0
        for app in self.apps:
            path = self.gdb.cwd / Path(app.file)
            sz = path.stat().st_size

            self.alias(app.file, offset=offset, size=sz)
            self.app("ramdisk", app.file, "ddr", "ddr")

            offset += self._aligned_app_size(path)


class ARMV7R5FZynqmpQemuTarget(QemuTarget):
    name = "armv7r5f-zynqmp-qemu"
    rootfs = False

    def __init__(self):
        super().__init__("armv7r5f-zynqmp-qemu-test.sh")

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()

    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter))

        if test.bootloader is not None:
            app_loader = None

            if test.bootloader.apps:
                app_loader = ARMV7R5FPloAppLoader(
                    dut=self.dut,
                    apps=test.bootloader.apps,
                    # port from armv7r5f-zynqmp-qemu-test.sh
                    gdb=GdbInteractive(
                        port=2024,
                        cwd=self.root_dir() / test.shell.path,
                        init_commands=[
                            "set architecture aarch64",
                            "set remote target-features-packet off",
                        ],
                    ),
                )

            builder.add(PloHarness(self.dut, app_loader=app_loader))

        if test.shell is not None:
            builder.add(
                ShellHarness(
                    self.dut,
                    self.shell_prompt,
                    test.shell.cmd,
                    prompt_timeout=self.prompt_timeout,
                    suppress_dmesg=False,
                )
            )
        else:
            builder.add(TestStartRunningHarness())

        builder.add(test.harness)

        return builder.get_harness()
