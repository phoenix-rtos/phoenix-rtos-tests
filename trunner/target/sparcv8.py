import subprocess
import time
from typing import Callable, Optional

import pexpect
import pexpect.fdpexpect
import usb.core

from trunner.dut import SerialDut
from trunner.harness import (
    HarnessBuilder,
    PloPhoenixdAppLoader,
    PloHarness,
    Rebooter,
    ShellHarness,
    FlashError,
    TestStartRunningHarness,
)
from trunner.host import Host
from trunner.tools import Phoenixd
from trunner.types import TestResult, TestOptions
from .base import TargetBase, find_port


class GR716Rebooter(Rebooter):
    def _reboot_soft(self):
        self._reboot_hard()
        # TODO implement soft restarts after implementing flash memory
        # self.host.set_reset(1)
        # time.sleep(0.5)
        # self.dut.clear_buffer()
        # self.host.set_reset(0)
        # time.sleep(0.25)

    def _reboot_hard(self):
        # Make sure that that reset pin is in low state
        self.host.set_reset(0)
        self.host.set_power(False)
        time.sleep(0.75)
        self.host.set_power(True)
        time.sleep(0.5)


class GR716Target(TargetBase):
    def __init__(self, host: Host, port: str, baudrate: int = 115200):
        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.rebooter = GR716Rebooter(host, self.dut)
        super().__init__()

    @classmethod
    def from_context(cls, ctx):
        return cls(ctx.host, ctx.port, ctx.baudrate)

    def MimasCustomUpload(self, boot_dir: str, image: str, vid: hex, pid: hex):
        # using usb.core to fully reset device responsible for phoenixd
        usb.core.find(idVendor=vid, idProduct=pid).reset()

        self.rebooter()

        # parsing passed hex values as string for find_port
        port = str(hex(vid))[2:] + ":" + str(hex(pid))[2:]

        subprocess.Popen(
            f"phoenixd -p {find_port(port)} -b 115200 -s {str(boot_dir)}/.",
            shell=True,
            cwd=".",
            start_new_session=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
            text=True,
        )

        while True:
            inChar = self.dut.read()
            inChar += inChar

            if "Bootloader" in inChar:
                with open(f"{boot_dir}/{image}", "rb") as fd:
                    filebuff = fd.read()
                    time.sleep(1)
                    self.dut.serial.write(filebuff)

                    try:
                        self.dut.expect_exact("(psh)%", timeout=80)
                        fd.close()
                        break

                    except pexpect.TIMEOUT as e:
                        fd.close()
                        raise FlashError(
                            msg=str(e),
                            output=e.stdout.decode("ascii") if e.stdout else None,
                        ) from e

    def flash_dut(self):
        self.MimasCustomUpload(
            boot_dir=self.boot_dir(), image="plo.img", vid=0x067B, pid=0x2303
        )

    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        builder = HarnessBuilder()

        if test.should_reboot:
            # TODO implement soft restarts after implementing flash memory
            self.dut.send("\r\n")

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


class MimasSparcV8EvkTarget(GR716Target):
    name = "sparcv8leon3-gr716-mimas"
    rootfs = False
    experimental = True

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if not port:
            port = find_port("10c4:ea60")  # vid:pid for communication (PLO usb/uart)

        super().__init__(host, port, baudrate)
