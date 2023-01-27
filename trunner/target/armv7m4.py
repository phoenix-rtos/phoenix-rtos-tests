import subprocess
import time
from pathlib import Path
from typing import Callable, Optional, Sequence

import pexpect.fdpexpect
import serial

from trunner.config import TestContext
from trunner.dut import Dut, PortError
from trunner.host import Host
from trunner.harness import (
    HarnessBase,
    PloInterface,
    VoidHarness,
    PloHarness,
    ShellHarness,
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
    pass


class OpenocdGdbServerHarness(HarnessBase):
    def __call__(self):
        with OpenocdGdbServer(interface="stlink", target="stm32l4x").run():
            return self.harness()


class STM32L4x6PloAppLoader(HarnessBase, PloInterface):
    def __init__(self, dut: Dut, apps: Sequence[AppOptions], gdb: GdbInteractive):
        self.dut = dut
        self.apps = apps
        self.gdb = gdb
        self.load_offset = 0x30000
        self.ram_addr = 0x20000000
        self.page_sz = 0x200
        super().__init__()

    def _app_size(self, path: Path):
        sz = path.stat().st_size
        offset = self.page_sz - (sz % self.page_sz) if sz % self.page_sz != 0 else 0
        return sz + offset

    def __call__(self):
        # First, load the apps into ram memory using gdb
        with self.gdb.run():
            offset = self.load_offset
            self.gdb.connect()

            for app in self.apps:
                path = self.gdb.cwd / Path(app.file)
                sz = self._app_size(path)

                self.gdb.load(path, self.ram_addr + offset)
                offset += sz

            self.gdb.cont()

            # When closing right after continuing plo will get stuck
            time.sleep(0.5)

        # Secondly, map loaded binaries as runnable programs
        offset = self.load_offset
        for app in self.apps:
            path = self.gdb.cwd / Path(app.file)
            sz = self._app_size(path)

            self.alias(app.file, offset=offset, size=sz)
            self.app("ramdev", app.file, "ram", "ram")

            offset += sz


class STM32L4x6Target(TargetBase):
    name = "armv7m4-stm32l4x6-nucleo"
    rootfs = False
    experimental = False
    image_file = "phoenix.disk"
    kernel_addr = 0x08000000

    class fdspawncustom(pexpect.fdpexpect.fdspawn):
        """
        The current UART implementation on the STM32L4 breaks when bytes are sent too fast.
        To alleviate this problem, we override the pexpect class and introduce a delay in the send function.
        """

        def send(self, s):
            ret = 0
            for c in s:
                ret += super().send(c)
                time.sleep(0.03)

            return ret

    class STM32L4SerialDut(Dut):
        def __init__(self, port: str, baudrate: int, *args, **kwargs):
            super().__init__()
            try:
                self.serial = serial.Serial(port, baudrate)
            except serial.SerialException as e:
                raise PortError(str(e)) from e

            self.pexpect_proc = STM32L4x6Target.fdspawncustom(self.serial, *args, **kwargs)

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if port is None:
            # Try to find USB-Serial controller
            port = find_port("USB-Serial|UART")

        self.dut = STM32L4x6Target.STM32L4SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
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
                    f"program {self.image_file} {self.kernel_addr:#x} verify reset exit",
                ],
                capture_output=True,
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

    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.chain(RebooterHarness(self.rebooter, hard=False))

        if test.bootloader is not None and test.bootloader.apps:
            # We are going to load some apps, so run gdb server
            builder.chain(OpenocdGdbServerHarness())

            # Running gdb server causes the board restart. We are left with unknown
            # content in the dut buffer. Reboot one more time using rebooter to make
            # sure buffer is cleared.
            builder.chain(RebooterHarness(self.rebooter, hard=False))

        if test.bootloader is not None:
            app_loader = None

            if test.bootloader.apps:
                app_loader = STM32L4x6PloAppLoader(
                    dut=self.dut,
                    apps=test.bootloader.apps,
                    gdb=GdbInteractive(port=3333, cwd=self.bin_dir()),
                )

            builder.chain(PloHarness(self.dut, app_loader=app_loader))

        if test.shell is not None:
            builder.chain(ShellHarness(self.dut, self.shell_prompt, test.shell.cmd))

        builder.chain(VoidHarness())

        setup = builder.get_harness()

        assert test.harness is not None
        run = test.harness

        # In the case we are loading apps using OpenGdbServer we don't want to keep it
        # open during a test execution. So first setup the device and then execute the test.
        def test_harness():
            setup()
            return run()

        return test_harness
