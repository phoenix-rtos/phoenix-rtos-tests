import subprocess
import time
from typing import Callable, Optional

from trunner.config import TestContext
from trunner.dut import SerialDut
from trunner.harness import (
    HarnessBuilder,
    PloInterface,
    PloImageLoader,
    PloImageProperty,
    ShellHarness,
    Rebooter,
    RebooterHarness,
    FlashError,
)
from trunner.host import Host
from trunner.tools import JLinkGdbServer, Phoenixd
from trunner.types import TestResult, TestOptions
from .base import TargetBase, find_port


class ARMv7A9TargetRebooter(Rebooter):
    def _reboot_soft(self):
        self._reboot_hard()

    def _reboot_hard(self):
        self.host.set_power(False)
        # optimal power off time to prevent sustaining chips, e.g. flash memory, related to #540 issue
        time.sleep(0.75)
        self.dut.clear_buffer()
        self.host.set_power(True)
        time.sleep(0.05)

    def _set_flash_mode(self, flash):
        self.host.set_flash_mode(not flash)


class ARMv7A9Target(TargetBase):
    image = PloImageProperty(file="phoenix.disk", source="usb0", memory_bank="flash0")

    def __init__(self, host: Host, port: str, baudrate: int = 115200):
        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.rebooter = ARMv7A9TargetRebooter(host, self.dut)
        super().__init__()

    def flash_dut(self):
        def gdb_plo_loader():
            """Loads plo image to RAM using gdb."""
            script = f"{self._project_dir()}/phoenix-rtos-build/scripts/upload-zynq7000.gdb"
            gdbserver = JLinkGdbServer("Zynq 7020")
            plo = PloInterface(self.dut)

            with gdbserver.run():
                try:
                    subprocess.run(
                        ["gdb-multiarch", "plo-gdb.elf", "-x", script, "-batch"],
                        encoding="ascii",
                        capture_output=True,
                        check=True,
                        timeout=20,
                        cwd=self.boot_dir(),
                    )
                except FileNotFoundError as e:
                    raise FlashError(msg=str(e)) from e
                except subprocess.CalledProcessError as e:
                    raise FlashError(msg=str(e), output=e.stdout) from e
                except subprocess.TimeoutExpired as e:
                    raise FlashError(msg=str(e), output=e.stdout.decode("ascii") if e.stdout else None) from e

            plo.enter_bootloader()
            plo.wait_prompt()
            # This erase command is not directly related to the plo loading.
            # We use this callback to clear the flash memory for phoenix image.
            # This action is unique to zynq-7000 target.
            plo.erase(device=self.image.memory_bank, offset=0x800000, size=0x1000000, timeout=80)

        loader = PloImageLoader(
            dut=self.dut,
            rebooter=self.rebooter,
            image=self.image,
            plo_loader=gdb_plo_loader,
            phoenixd=Phoenixd(directory=self.boot_dir()),
        )

        loader()

    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.chain(RebooterHarness(self.rebooter))

        if test.shell is not None:
            builder.chain(ShellHarness(self.dut, self.shell_prompt, test.shell.cmd))

        builder.chain(test.harness)

        return builder.get_harness()


class Zynq7000ZedboardTarget(ARMv7A9Target):
    name = "armv7a9-zynq7000-zedboard"

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if port is None:
            port = find_port("04b4:0008")

        super().__init__(host, port, baudrate)

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls(ctx.host, ctx.port, ctx.baudrate)
