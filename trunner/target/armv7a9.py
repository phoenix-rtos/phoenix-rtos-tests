from abc import abstractmethod
import subprocess
import time
from typing import Callable, Optional

from trunner.ctx import TestContext
from trunner.dut import Dut, SerialDut
from trunner.harness import (
    HarnessBuilder,
    PloInterface,
    PloJffsImageProperty,
    PloImageLoader,
    ShellHarness,
    TestStartRunningHarness,
    Rebooter,
    RebooterHarness,
    FlashError,
    PloJffs2CleanmarkerSpec,
)
from trunner.harness import TerminalHarness
from trunner.host import Host
from trunner.tools import Phoenixd, OpenocdGdbServer, wait_for_vid_pid
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


class ZynqZedboardGdbPloLoader(TerminalHarness, PloInterface):
    def __init__(self, dut: Dut, script: str, cwd: Optional[str] = None):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.script = script
        self.cwd = cwd
        self.gdbserver = OpenocdGdbServer(board="digilent_zedboard")

    def __call__(self):
        """Loads plo image to RAM using gdb."""

        # after reboot we need to wait for smt2 device
        wait_for_vid_pid(vid=0x0403, pid=0x6014, timeout=5)

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

        self.wait_prompt()


class ARMv7A9Target(TargetBase):
    image: PloJffsImageProperty

    def __init__(self, host: Host, port: str, baudrate: int = 115200):
        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.rebooter = ARMv7A9TargetRebooter(host, self.dut)
        super().__init__()

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls(ctx.host, ctx.port, ctx.baudrate)

    @abstractmethod
    def flash_dut(self):
        # for now there is no common flashing approach for all future armv7a9 targets
        pass

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


class Zynq7000ZedboardTarget(ARMv7A9Target):
    # Zynq7000Zedboard with system jffs2 use flash0 as space to hold data
    image = PloJffsImageProperty(
        file="phoenix.disk",
        source="usb0",
        memory_bank="flash0",
        flash_device_id="2.0",
        cleanmarkers_args=PloJffs2CleanmarkerSpec(
            start_block=0x80,
            number_of_blocks=0x100,
            block_size=0x10000,
            cleanmarker_size=0x10,
        ),
        # Based on S25FL256S1 IC docs 4KB subsector erase time equals 0.4s max.
        # Blocksize set for jffs2 command is 64KB, so 16 sectors have to be erased for 1 block
        # That's why we set (0.65 * 16 / 2) as a timeout. The value is divided by 2 to early detect slowing the flash
        block_timeout=5.2,
    )
    name = "armv7a9-zynq7000-zedboard"

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if port is None:
            port = find_port("04b4:0008")

        super().__init__(host, port, baudrate)

    def flash_dut(self):
        plo_loader = ZynqZedboardGdbPloLoader(
            dut=self.dut,
            script=f"{self._project_dir()}/phoenix-rtos-build/scripts/upload-zynq7000-smt2.gdb",
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
