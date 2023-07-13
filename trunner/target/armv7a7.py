from typing import Callable, Optional

from trunner.dut import Dut, SerialDut
from trunner.harness import (
    HarnessBuilder,
    PloInterface,
    PloImageLoader,
    PloImageProperty,
    ShellHarness,
    Rebooter,
    RebooterHarness,
    PloJffs2CleanmarkerSpec,
)

from trunner.harness import TerminalHarness
from trunner.host import Host
from trunner.tools import Phoenixd, Psu
from trunner.types import TestResult, TestOptions
from .base import TargetBase, find_port


class ARMv7A7TargetRebooter(Rebooter):
    def _set_flash_mode(self, flash):
        self.host.set_flash_mode(not flash)


class PsuPloLoader(TerminalHarness, PloInterface):
    def __init__(self, dut: Dut, psu: Psu, flash_device_id: str, cleanmarkers_args: PloJffs2CleanmarkerSpec):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.psu = psu
        self.flash_device_id = flash_device_id
        self.cleanmarkers_args = cleanmarkers_args

    def __call__(self):
        """Loads plo image to RAM using psu tool and erases an area intended for rootfs."""
        self.psu.run()
        self.wait_prompt()

        # Device id and cleanmarkers arguments are set based on target configuration in _targets/_projects.
        self.jffs2(self.flash_device_id, True, self.cleanmarkers_args, 90)


class ARMv7A7Target(TargetBase, PloInterface, Rebooter):
    plo_psu_script = "plo-ram.sdp"

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
            flash_device_id=self.flash_device_id,
            cleanmarkers_args=self.cleanmarkers_spec,
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
    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter))

        if test.shell is not None:
            builder.add(ShellHarness(self.dut, self.shell_prompt, test.shell.cmd))

        builder.add(test.harness)

        return builder.get_harness()


class IMX6ULLEvkTarget(ARMv7A7Target):
    # IMX6ULL with system jffs2 use nor0 as space to hold data
    image = PloImageProperty(file="phoenix.disk", source="usb0", memory_bank="nor0")
    name = "armv7a7-imx6ull-evk"
    cleanmarkers_spec = PloJffs2CleanmarkerSpec(
        start_block=0x10,
        number_of_blocks=0x1F0,
        block_size=0x10000,
        cleanmarker_size=0x10,
    )
    flash_device_id = "2.0"

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if not port:
            port = find_port("10c4:ea60")  # vid:pid Product=CP2102 USB to UART Bridge Controller for imx6ull-evk

        super().__init__(host, port, baudrate)
