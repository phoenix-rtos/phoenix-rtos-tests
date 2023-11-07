from typing import Callable, Optional
from trunner.dut import SerialDut
from trunner.harness import (
    HarnessBuilder,
    PloImageLoader,
    PloImageProperty,
    PloPhoenixdAppLoader,
    PloHarness,
    Rebooter,
    RebooterHarness,
    ShellHarness,
    TestStartRunningHarness,
)
from trunner.host import Host
from trunner.tools import Phoenixd, Psu
from trunner.types import TestOptions, TestResult
from .base import TargetBase, PsuPloLoader, find_port


class ARMv7M7TargetRebooter(Rebooter):
    # TODO add text mode
    def _set_flash_mode(self, flash):
        # setting the flash mode pin on this specific test unit to high causes switch to flash memory mode
        self.host.set_flash_mode(not flash)

    def _reboot_hard(self):
        # FIXME due to problems with relay use only soft reboot
        self._reboot_soft()


class ARMv7M7Target(TargetBase):
    image = PloImageProperty(file="phoenix.disk", source="usb0", memory_bank="flash0")
    plo_psu_script = "plo-ram.sdp"
    rootfs = False

    def __init__(self, host: Host, port: str, baudrate: int = 115200):
        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.rebooter = ARMv7M7TargetRebooter(host, self.dut)
        super().__init__()

    @classmethod
    def from_context(cls, ctx):
        return cls(ctx.host, ctx.port, ctx.baudrate)

    def flash_dut(self):
        plo_loader = PsuPloLoader(
            dut=self.dut,
            psu=Psu(self.plo_psu_script, self.boot_dir()),
        )

        loader = PloImageLoader(
            dut=self.dut,
            rebooter=self.rebooter,
            image=self.image,
            plo_loader=plo_loader,
            phoenixd=Phoenixd(directory=self.boot_dir()),
        )

        loader()

    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter, hard=False))

        if test.bootloader is not None:
            app_loader = None

            if test.bootloader.apps:
                app_loader = PloPhoenixdAppLoader(
                    dut=self.dut,
                    apps=test.bootloader.apps,
                    phoenixd=Phoenixd(directory=self.root_dir() / test.shell.path),
                )

            builder.add(PloHarness(self.dut, app_loader=app_loader))

        if test.shell is not None:
            builder.add(ShellHarness(self.dut, self.shell_prompt, test.shell.cmd))
        else:
            builder.add(TestStartRunningHarness())

        builder.add(test.harness)

        return builder.get_harness()


class IMXTarget(ARMv7M7Target):
    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if not port:
            port = find_port("0D28:0204")  # vid:pid

        super().__init__(host, port, baudrate)


class IMXRT106xEvkTarget(IMXTarget):
    name = "armv7m7-imxrt106x-evk"
    image = PloImageProperty(file="phoenix.disk", source="usb0", memory_bank="flash1")


class IMXRT117xEvkTarget(IMXTarget):
    name = "armv7m7-imxrt117x-evk"
    image = PloImageProperty(file="phoenix.disk", source="usb0", memory_bank="flash0")

    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        # FIXME due to https://github.com/phoenix-rtos/phoenix-rtos-project/issues/580 always reboot
        test.should_reboot = True

        return super().build_test(test)
