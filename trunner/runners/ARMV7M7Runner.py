#
# Phoenix-RTOS test runner
#
# ARMV7M7 runner
#
# Copyright 2021 Phoenix Systems
# Authors: Jakub Sarzyński, Damian Loewnau
#

import time
import sys
import select

from pexpect.exceptions import TIMEOUT, EOF
from trunner.tools.color import Color
from .common import LOG_PATH, PloError, PloTalker, DeviceRunner
from .common import GPIO, rootfs
from .wrappers import Phoenixd, PhoenixdError, error_msg
from .flasher import NXPSerialFlasher


class RebootError(Exception):
    def __init__(self, message):
        msg = Color.colorify("REBOOT ERROR:\n", Color.BOLD)
        msg += str(message) + '\n'
        super().__init__(msg)


def hostpc_reboot(serial_downloader=False):
    """Reboots a tested device by a human."""
    if serial_downloader:
        print("\n\tPlease change the board's boot mode to serial download, reset the board and press enter")
    else:
        print("\n\tPlease change the board's boot mode to internal/external flash if the current mode is different.\n\
        Next, reset the board and press enter")
    print("\tNote that You should press the enter key just after reset button")

    if sys.stdin in select.select([sys.stdin], [], [], 30)[0]:
        sys.stdin.readline()
    else:
        raise RebootError('It took too long to wait for a key pressing')


class ARMV7M7Runner(DeviceRunner):
    """This class provides interface to run tests on targets with armv7m7 architecture"""

    # redefined by target runners
    SDP = None
    IMAGE = None

    def __init__(self, target, serial, is_rpi_host=True, log=False):
        # has to be defined before super, because Runner constructor calls set_status, where it's used
        self.is_rpi_host = is_rpi_host
        if self.is_rpi_host:
            self.reset_gpio = GPIO(17)
            self.reset_gpio.high()
            self.power_gpio = GPIO(2)
            self.power_gpio.high()
            self.boot_gpio = GPIO(4)
            self.logpath = LOG_PATH
        super().__init__(target, serial, is_rpi_host, log)
        # default values, redefined by specified target runners
        self.phoenixd_port = None
        self.is_cut_power_used = False

    def _restart_by_poweroff(self):
        pass

    def _restart_by_jtag(self):
        self.reset_gpio.low()
        time.sleep(0.050)
        self.reset_gpio.high()

    def rpi_reboot(self, serial_downloader=False, cut_power=False):
        """Reboots a tested device using Raspberry Pi as host-generic-pc."""
        if serial_downloader:
            self.boot_gpio.low()
        else:
            self.boot_gpio.high()

        if cut_power:
            self._restart_by_poweroff()
        else:
            self._restart_by_jtag()

    def reboot(self, serial_downloader=False, cut_power=False):
        """Reboots a tested device."""
        if self.is_rpi_host:
            self.rpi_reboot(serial_downloader, cut_power)
        else:
            hostpc_reboot(serial_downloader)

    def flash(self):
        flasher = NXPSerialFlasher(self)
        flasher.flash()

    def load(self, test):
        """Loads test ELF into syspage using plo"""
        phd = None
        load_dir = str(rootfs(test.target) / 'bin')
        try:
            self.reboot(cut_power=self.is_cut_power_used)
            with PloTalker(self.serial_port) as plo:
                if self.logpath:
                    plo.plo.logfile = open(self.logpath, "a")
                plo.interrupt_counting()
                plo.wait_prompt()
                if not test.exec_cmd and not test.syspage_prog:
                    # We got plo prompt, we are ready for sending the "go!" command.
                    return True

                with Phoenixd(self.target, self.phoenixd_port, dir=load_dir) as phd:
                    prog = test.exec_cmd[0] if test.exec_cmd else test.syspage_prog
                    plo.app('usb0', prog, 'ocram2', 'ocram2')
        except (TIMEOUT, EOF, PloError, PhoenixdError, RebootError) as exc:
            if isinstance(exc, TIMEOUT) or isinstance(exc, EOF):
                test.exception = Color.colorify('EXCEPTION PLO\n', Color.BOLD)
                test.handle_pyexpect_error(plo.plo, exc)
            else:  # RebootError, PloError or PhoenixdError
                test.exception = str(exc)
                test.fail()
                if phd:
                    test.exception = error_msg('phoenixd', test.exception, phd.output())
            return False

        return True

    def run(self, test):
        if test.skipped():
            return

        if not self.load(test):
            return
        super().run(test)
