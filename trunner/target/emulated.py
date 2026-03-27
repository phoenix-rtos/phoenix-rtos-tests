from abc import abstractmethod
from pathlib import Path
from typing import Callable, Optional, Sequence, TextIO

from trunner.ctx import TestContext
from trunner.dut import Dut, QemuDut
from trunner.harness import (
    HarnessBuilder,
    PloHarness,
    PloRamSyspageLoader,
    RebooterHarness,
    ShellHarness,
    TestStartRunningHarness,
)
from trunner.tools import GdbInteractive
from trunner.types import AppOptions, FileOptions, TestOptions, TestResult
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


class ARMV7R5FSyspageLoader(PloRamSyspageLoader):
    """Loads app binaries and file blobs into syspage via GDB on armv7r5f-zynqmp-qemu."""

    source_device = "ramdisk"
    destination_map = "ddr"
    ramdisk_addr = 0x8000000
    page_sz = 0x200

    def __init__(
        self,
        dut: Dut,
        apps: Sequence[AppOptions],
        gdb: GdbInteractive,
        files: Sequence[FileOptions] = (),
        root_dir: Optional[Path] = None,
    ):
        super().__init__(dut, apps, files, root_dir)
        self.gdb = gdb

    def __call__(self) -> None:
        with self.gdb.run():
            offset = 0
            apps_offset = offset
            self.gdb.connect()

            for app in self.apps:
                path = self.gdb.cwd / Path(app.file)
                aligned_sz = self._aligned_size(path)
                self.gdb.load(path, self.ramdisk_addr + offset)
                offset += aligned_sz

            files_offset = offset
            for f in self.files:
                path = self._root_dir / Path(f.file).relative_to("/")
                aligned_sz = self._aligned_size(path)
                self.gdb.load(path, self.ramdisk_addr + offset)
                offset += aligned_sz

        self._register_apps_in_plo(apps_offset, app_host_dir=self.gdb.cwd)
        self._register_files_in_plo(files_offset)


class ARMV7R5FZynqmpQemuTarget(QemuTarget):
    name = "armv7r5f-zynqmp-qemu"
    rootfs = False
    # Matches the GDB port defined in armv7r5f-zynqmp-qemu-test.sh
    gdb_port = 2024

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
            syspage_loader = None

            if test.bootloader.apps or test.bootloader.files:
                syspage_loader = ARMV7R5FSyspageLoader(
                    dut=self.dut,
                    apps=test.bootloader.apps,
                    gdb=GdbInteractive(
                        port=self.gdb_port,
                        cwd=self.root_dir() / test.shell.path,
                    ),
                    files=test.bootloader.files,
                    root_dir=self.root_dir(),
                )

            builder.add(PloHarness(self.dut, syspage_loader=syspage_loader))

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
