from typing import Callable, Optional

from trunner.dut import SerialDut
from trunner.harness import (
    HarnessBuilder,
    PloInterface,
    PloImageLoader,
    PloJffsImageProperty,
    ShellHarness,
    TestStartRunningHarness,
    Rebooter,
    RebooterHarness,
    PloJffs2CleanmarkerSpec,
)

from trunner.host import Host
from trunner.tools import Phoenixd, Psu
from trunner.types import TestResult, TestOptions
from .base import TargetBase, PsuPloLoader, find_port


class ARMv7A7TargetRebooter(Rebooter):
    def _set_flash_mode(self, flash):
        self.host.set_flash_mode(not flash)


class ARMv7A7Target(TargetBase, PloInterface, Rebooter):
    plo_psu_script: str = "plo-ram.sdp"
    image: PloJffsImageProperty

    def __init__(self, host: Host, port: str, baudrate: int = 115200):
        dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        PloInterface.__init__(self, dut)
        self.rebooter = ARMv7A7TargetRebooter(host, self.dut)
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

    # Setup environment for tests
    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter))

        if test.shell is not None:
            builder.add(ShellHarness(self.dut, self.shell_prompt, test.shell.cmd))
        else:
            builder.add(TestStartRunningHarness())

        builder.add(test.harness)

        return builder.get_harness()


class IMX6ULLEvkTarget(ARMv7A7Target):
    # IMX6ULL with system jffs2 use nor0 as space to hold data
    image = PloJffsImageProperty(
        file="phoenix.disk",
        source="usb0",
        memory_bank="nor0",
        flash_device_id="2.0",
        cleanmarkers_args=PloJffs2CleanmarkerSpec(
            start_block=0x10,
            number_of_blocks=0x1F0,
            block_size=0x10000,
            cleanmarker_size=0x10,
        ),
        # Based on MT25QL256 IC docs 4KB subsector erase time equals 0.4s max.
        # Blocksize set for jffs2 command is 64KB, so 16 sectors have to be erased for 1 block
        # That's why we set (0.4 * 16 / 2) as a timeout. The value is divided by 2 to early detect slowing the flash
        block_timeout=3.2,
    )
    name = "armv7a7-imx6ull-evk"

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if not port:
            port = find_port("10c4:ea60")  # vid:pid Product=CP2102 USB to UART Bridge Controller for imx6ull-evk

        super().__init__(host, port, baudrate)
