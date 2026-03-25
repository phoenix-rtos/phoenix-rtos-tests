from pathlib import Path
from typing import Optional, Sequence, TextIO

from trunner.ctx import TestContext
from trunner.dut import Dut, SerialDut
from trunner.host import Host
from trunner.harness import (
    PloHarness,
    PloRamSyspageLoader,
    ShellHarness,
    TestStartRunningHarness,
    Rebooter,
    RebooterHarness,
    HarnessBuilder,
    FlashError,
)
from trunner.tools import PyocdProcess, PyocdError
from trunner.types import AppOptions, FileOptions, TestOptions
from .base import TargetBase, find_port


class ARMv8M33Rebooter(Rebooter):
    # NOTE: changing boot modes not needed/supported for this target

    def __call__(self, flash=False, hard=False):
        """Sets flash mode and perform hard or soft & debugger reboot based on `hard` flag."""

        if hard and self.host.has_gpio():
            self._reboot_dut_gpio(hard=hard)
        else:
            self._reboot_by_debugger()

    def _reboot_by_debugger(self):
        PyocdProcess(target="mcxn947").reset()


class MCXN947SyspageLoader(PloRamSyspageLoader):
    """Loads app binaries and file blobs into RAM via pyocd and registers them with PLO on MCXN947."""

    source_device = "ramdev"
    destination_map = "ram"
    load_offset = 0x25000
    ram_addr = 0x20000000
    page_sz = 0x200

    def __init__(
        self,
        dut: Dut,
        apps: Sequence[AppOptions],
        app_host_dir: Path,
        files: Sequence[FileOptions] = (),
        root_dir: Optional[Path] = None,
    ):
        super().__init__(dut, apps, files, root_dir)
        self.app_host_dir = app_host_dir

    def __call__(self) -> None:
        offset = self.load_offset

        apps_offset = offset
        for app in self.apps:
            path = self.app_host_dir / Path(app.file)
            PyocdProcess(target="mcxn947", extra_args=["--format=bin", "--no-reset"], cwd=self.app_host_dir).load(
                load_file=app.file, load_offset=self.ram_addr + offset
            )
            offset += self._aligned_size(path)

        files_offset = offset
        for f in self.files:
            path = self._root_dir / Path(f.file).relative_to("/")
            PyocdProcess(target="mcxn947", extra_args=["--format=bin", "--no-reset"], cwd=self._root_dir).load(
                load_file=str(Path(f.file).relative_to("/")), load_offset=self.ram_addr + offset
            )

            offset += self._aligned_size(path)

        self._register_apps_in_plo(apps_offset, self.app_host_dir)
        self._register_files_in_plo(files_offset)


class MCXN94xTarget(TargetBase):
    name = "armv8m33-mcxn94x-frdm"
    rootfs = False
    image_file = "phoenix.disk"

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if port is None:
            # Try to find USB-Serial controller
            port = find_port("MCU-LINK")

        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.rebooter = ARMv8M33Rebooter(host, self.dut)
        super().__init__()

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls(ctx.host, ctx.port, ctx.baudrate)

    def flash_dut(self, host_log: TextIO):
        try:
            PyocdProcess(
                target="mcxn947", extra_args=["--format=bin", "--erase=chip"], host_log=host_log, cwd=self.boot_dir()
            ).load(load_file=self.image_file)

        except FileNotFoundError as e:
            raise FlashError(msg=str(e)) from e
        except PyocdError as e:
            raise FlashError(msg=str(e)) from e

    def build_test(self, test: TestOptions):
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter))

        if test.bootloader is not None:
            syspage_loader = None

            if test.bootloader.apps or test.bootloader.files:
                syspage_loader = MCXN947SyspageLoader(
                    dut=self.dut,
                    apps=test.bootloader.apps,
                    app_host_dir=self.root_dir() / test.shell.path,
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
                    # /dev/klogctl support missing on this target.
                    # related to issue :https://github.com/phoenix-rtos/phoenix-rtos-project/issues/948
                    suppress_dmesg=False,
                )
            )
        else:
            builder.add(TestStartRunningHarness())

        builder.add(test.harness)

        return builder.get_harness()
