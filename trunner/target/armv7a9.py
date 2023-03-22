import subprocess
import time
from abc import abstractmethod
from typing import Callable, Optional

from trunner.ctx import TestContext
from trunner.dut import Dut, SerialDut
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
from trunner.harness import TerminalHarness
from trunner.host import Host
from trunner.tools import JLinkGdbServer, Phoenixd
from trunner.types import TestResult, TestOptions
from .base import TargetBase, find_port


class ARMv7A9TargetRebooter(Rebooter):
    # TODO add text mode reboot

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


class ZynqGdbPloLoader(TerminalHarness, PloInterface):
    def __init__(self, dut: Dut, script: str, cwd: Optional[str] = None):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.script = script
        self.cwd = cwd
        self.gdbserver = JLinkGdbServer("Zynq 7020")

    def __call__(self):
        """Loads plo image to RAM using gdb."""

        with self.gdbserver.run():
            try:
                subprocess.run(
                    ["gdb-multiarch", "plo-gdb.elf", "-x", self.script, "-batch"],
                    encoding="ascii",
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    check=True,
                    timeout=20,
                    cwd=self.cwd,
                )
            except FileNotFoundError as e:
                raise FlashError(msg=str(e)) from e
            except subprocess.CalledProcessError as e:
                raise FlashError(msg=str(e), output=e.stdout) from e
            except subprocess.TimeoutExpired as e:
                raise FlashError(msg=str(e), output=e.stdout.decode("ascii") if e.stdout else None) from e

        self.enter_bootloader()
        self.wait_prompt()
        # This erase command is not directly related to the plo loading.
        # We use this callback to clear the flash memory for phoenix image.
        # This action is unique to zynq-7000 target.
        self.erase(device=Zynq7000ZedboardTarget.image.memory_bank, offset=0x800000, size=0x1000000, timeout=80)


class ARMv7A9Target(TargetBase):
    image = PloImageProperty(file="phoenix.disk", source="usb0", memory_bank="flash0")

    def __init__(self, host: Host, port: str, baudrate: int = 115200):
        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.rebooter = ARMv7A9TargetRebooter(host, self.dut)
        super().__init__()

    @classmethod
    @abstractmethod
    def from_context(cls, _: TestContext):
        pass

    @abstractmethod
    def flash_dut(self):
        pass

    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter))

        if test.shell is not None:
            builder.add(ShellHarness(self.dut, self.shell_prompt, test.shell.cmd))

        builder.add(test.harness)

        return builder.get_harness()


class Zynq7000ZedboardTarget(ARMv7A9Target):
    name = "armv7a9-zynq7000-zedboard"

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if port is None:
            port = find_port("04b4:0008")

        super().__init__(host, port, baudrate)

    def flash_dut(self):
        plo_loader = ZynqGdbPloLoader(
            dut=self.dut,
            script=f"{self._project_dir()}/phoenix-rtos-build/scripts/upload-zynq7000.gdb",
            cwd=self.boot_dir(),
        )

        loader = PloImageLoader(
            dut=self.dut,
            rebooter=self.rebooter,
            image=self.image,
            plo_loader=plo_loader,
            phoenixd=Phoenixd(directory=self.boot_dir()),
        )

        loader()

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls(ctx.host, ctx.port, ctx.baudrate)
