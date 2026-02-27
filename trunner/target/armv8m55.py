import time
from pathlib import Path
from typing import Callable, Optional, Sequence, TextIO

from trunner.ctx import TestContext
from trunner.dut import Dut, SerialDut
from trunner.host import Host
from trunner.harness import (
    IntermediateHarness,
    TerminalHarness,
    PloInterface,
    PloHarness,
    PloError,
    ShellHarness,
    TestStartRunningHarness,
    Rebooter,
    RebooterHarness,
    HarnessBuilder,
    FlashError,
)
from trunner.tools import GdbInteractive, OpenocdGdbServer, OpenocdError
from trunner.types import AppOptions, TestOptions, TestResult
from .base import TargetBase, find_port


class ARMv8M55Rebooter(Rebooter):
    # NOTE: changing boot modes not needed/supported for this target

    def __init__(self, host, dut, gdb):
        super().__init__(host, dut)
        self.gdb = gdb

    def __call__(self, flash=False, hard=False):
        """Sets flash mode and perform hard or soft & debugger reboot based on `hard` flag."""

        if hard and self.host.has_gpio():
            self._reboot_dut_gpio(hard=hard)
        else:
            self._reboot_by_debugger()

    def _reboot_by_debugger(self):
        with self.gdb.run():
            self.gdb.set_architecture("arm")
            self.gdb.connect()
            self.gdb.reset()
            self.gdb.cont()
            time.sleep(0.5)


class STM32N6STLinkGdbServerHarness(IntermediateHarness):
    """Harness that runs other harness in ST-LINK gdb server context.

    It calls passed harness under gdb server context and then continue normal
    harness execution flow.

    Attributes:
        harness: Harness that will be run under gdb server context.

    """

    def __init__(self, harness: Callable[[TestResult], TestResult]):
        super().__init__()
        self.harness = harness

    def __call__(self, result: TestResult) -> TestResult:
        with OpenocdGdbServer(interface="stlink-dap", target="stm32n6x").run():
            self.harness(result)

        return self.next_harness(result)


class STM32N6PloAppLoader(TerminalHarness, PloInterface):
    def __init__(self, dut: Dut, apps: Sequence[AppOptions], gdb: GdbInteractive):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.dut = dut
        self.apps = apps
        self.gdb = gdb
        self.load_offset = 0x00
        self.ram_addr = 0x34100000
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
            self.gdb.set_architecture("arm")
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
            self.app("ramdisk", app.file, "axi_app", "axi_app")

            offset += self._aligned_app_size(path)


class STM32N6Target(TargetBase):
    name = "armv8m55-stm32n6-nucleo"
    rootfs = False
    experimental = False
    image_file = "phoenix.disk.bin"
    plo_file = "plo-ram.elf"
    image_addr = 0x70000000
    ram_addr = 0x34100000

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if port is None:
            # Try to find ST-LINK
            port = find_port("ST-LINK")
        self.port = port
        self.host = host
        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.rebooter = ARMv8M55Rebooter(host, self.dut, GdbInteractive(port=3333))
        self.baudrate = baudrate
        super().__init__()

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls(ctx.host, ctx.port, ctx.baudrate)

    def flash_dut(self, host_log: TextIO):
        try:
            print("Set BOOT1 jumper to 1 and power cycle the board")
            input("Press Enter to continue...")
            with OpenocdGdbServer(interface="stlink-dap", target="stm32n6x").run():
                gdb = GdbInteractive(port=3333, cwd=self.boot_dir())
                with gdb.run():
                    plo = PloInterface(self.dut)
                    gdb.set_architecture("arm")
                    gdb.connect()
                    gdb.reset()
                    gdb.cont()
                    gdb.pause()

                    # Load PLO to RAM
                    gdb.load_program(self.plo_file)
                    gdb.cont()

                    # The board reboots after loading PLO for the first time,
                    # load it again if plo prompt doesn't appear
                    try:
                        plo.wait_prompt(timeout=2)
                    except PloError:
                        gdb.pause()
                        gdb.load_program(self.plo_file)
                        gdb.cont()

                        plo.wait_prompt()

                    # Load disk image to ramdisk
                    # TODO: load image in parts if it's larger than ramdisk size (512 KiB)
                    gdb.pause()
                    gdb.load(self.image_file, self.ram_addr)
                    gdb.cont()

                    # Copy from ramdisk to flash
                    # TODO: use image size rather  than ramdisk size
                    plo.erase("flash0", 0x0, 0x80000)
                    plo.copy("ramdisk", 0x0, "flash0", 0x0, 0x80000, 0x80000)

        except FileNotFoundError as e:
            raise FlashError(msg=str(e)) from e
        except OpenocdError as e:
            raise FlashError(msg=str(e)) from e
        print("Set BOOT1 jumper to 0 WITHOUT power cycling the board")
        input("Press Enter to continue...")

    def build_test(self, test: TestOptions):
        builder = HarnessBuilder()

        if test.should_reboot:
            self.rebooter = ARMv8M55Rebooter(self.host, self.dut, GdbInteractive(port=3333))
            builder.add(RebooterHarness(self.rebooter))

            app_loader = None

            if test.bootloader and test.bootloader.apps:
                app_loader = STM32N6PloAppLoader(
                    dut=self.dut,
                    apps=test.bootloader.apps,
                    gdb=GdbInteractive(port=3333, cwd=self.root_dir() / test.shell.path),
                )

            builder.add(PloHarness(self.dut, app_loader=app_loader))

            setup = builder.get_harness()
            builder = HarnessBuilder()
            builder.add(STM32N6STLinkGdbServerHarness(setup))

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
