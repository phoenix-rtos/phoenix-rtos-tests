import subprocess
import time
from pathlib import Path
from typing import Callable, Optional, Sequence

from trunner.ctx import TestContext
from trunner.dut import Dut, SerialDut
from trunner.host import Host
from trunner.harness import (
    IntermediateHarness,
    TerminalHarness,
    PloInterface,
    PloHarness,
    ShellHarness,
    TestStartRunningHarness,
    Rebooter,
    RebooterHarness,
    HarnessBuilder,
    FlashError,
)
from trunner.tools import GdbInteractive, OpenocdGdbServer
from trunner.types import AppOptions, TestOptions, TestResult
from .base import TargetBase, find_port


class ARMv7M4Rebooter(Rebooter):
    # TODO add text mode
    # NOTE: changing boot modes not needed/supported for this target
    pass


class STM32L4x6OpenocdGdbServerHarness(IntermediateHarness):
    """Harness that runs other harness in Opendocd gdb server context.

    It calls passed harness under gdb server context and then continue normal
    harness execution flow.

    Attributes:
        harness: Harness that will be run under gdb server context.

    """

    def __init__(self, harness: Callable[[TestResult], TestResult]):
        super().__init__()
        self.harness = harness

    def __call__(self, result: TestResult) -> TestResult:
        # Set of config parameters used in Openocd to flash up stm32l4a6
        openocd_args = ["-c", "reset_config srst_only srst_nogate connect_assert_srst", "-c", "init;reset"]

        with OpenocdGdbServer(interface="stlink", target="stm32l4x", extra_args=openocd_args).run():
            self.harness(result)

        return self.next_harness(result)


class STM32L4x6PloAppLoader(TerminalHarness, PloInterface):
    def __init__(self, dut: Dut, apps: Sequence[AppOptions], gdb: GdbInteractive):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.dut = dut
        self.apps = apps
        self.gdb = gdb
        self.load_offset = 0x30000
        self.ram_addr = 0x20000000
        self.page_sz = 0x200

    def _aligned_app_size(self, path: Path):
        sz = path.stat().st_size
        # round up to the size of the page
        offset = (self.page_sz - sz) % self.page_sz
        return sz + offset

    def __call__(self):
        # First, load the apps into ram memory using gdb
        with self.gdb.run():
            offset = self.load_offset
            self.gdb.connect()

            for app in self.apps:
                path = self.gdb.cwd / Path(app.file)
                sz = self._aligned_app_size(path)

                self.gdb.load(path, self.ram_addr + offset)
                offset += sz

            self.gdb.cont()

            # When closing right after continuing plo will get stuck
            time.sleep(0.5)

        # Secondly, map loaded binaries as runnable programs
        offset = self.load_offset
        for app in self.apps:
            path = self.gdb.cwd / Path(app.file)
            sz = path.stat().st_size

            self.alias(app.file, offset=offset, size=sz)
            self.app("ramdev", app.file, "ram", "ram")

            offset += self._aligned_app_size(path)


class STM32L4x6Target(TargetBase):
    name = "armv7m4-stm32l4x6-nucleo"
    rootfs = False
    experimental = False
    image_file = "phoenix.disk"
    image_addr = 0x08000000

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if port is None:
            # Try to find USB-Serial controller
            port = find_port("USB-Serial|UART")

        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.rebooter = ARMv7M4Rebooter(host, self.dut)
        super().__init__()

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls(ctx.host, ctx.port, ctx.baudrate)

    def flash_dut(self):
        try:
            subprocess.run(
                [
                    "openocd",
                    "-f",
                    "interface/stlink.cfg",
                    "-f",
                    "target/stm32l4x.cfg",
                    "-c",
                    "reset_config srst_only srst_nogate connect_assert_srst",
                    "-c",
                    f"program {self.image_file} {self.image_addr:#x} verify reset exit",
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=True,
                encoding="ascii",
                timeout=20,
                cwd=self.boot_dir(),
            )
        except FileNotFoundError as e:
            raise FlashError(msg=str(e)) from e
        except subprocess.CalledProcessError as e:
            raise FlashError(msg=str(e), output=e.stdout) from e
        except subprocess.TimeoutExpired as e:
            raise FlashError(msg=str(e), output=e.stdout.decode("ascii") if e.stdout else None) from e

    def build_test(self, test: TestOptions):
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter, hard=False))

        if test.bootloader is not None:
            app_loader = None

            if test.bootloader.apps:
                app_loader = STM32L4x6PloAppLoader(
                    dut=self.dut,
                    apps=test.bootloader.apps,
                    gdb=GdbInteractive(port=3333, cwd=self.root_dir() / test.shell.path),
                )

            builder.add(PloHarness(self.dut, app_loader=app_loader))

        if test.bootloader is not None and test.bootloader.apps:
            # In the case we are loading apps using OpenGdbServer we would like to run plo
            # in the gdb server context. Get the harness that we already build, pack it in gdb
            # server context and continue building harness
            setup = builder.get_harness()
            builder = HarnessBuilder()
            builder.add(STM32L4x6OpenocdGdbServerHarness(setup))

        if test.shell is not None:
            builder.add(
                ShellHarness(
                    self.dut,
                    self.shell_prompt,
                    test.shell.cmd,
                    # /dev/klogctl support missing on stm32.
                    # related to issue :https://github.com/phoenix-rtos/phoenix-rtos-project/issues/948
                    suppress_dmesg=False,
                )
            )
        else:
            builder.add(TestStartRunningHarness())

        builder.add(test.harness)

        return builder.get_harness()
