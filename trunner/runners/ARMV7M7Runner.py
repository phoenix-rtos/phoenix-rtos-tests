#
# Phoenix-RTOS test runner
#
# ARMV7M7 runner
#
# Copyright 2021 Phoenix SYstems
# Authors: Jakub Sarzyński, Damian Loewnau
#

import logging
import time
import sys
import select

from pexpect.exceptions import TIMEOUT, EOF
from trunner.tools.color import Color
from .common import LOG_PATH, Psu, Phoenixd, PhoenixdError, PloError, PloTalker, DeviceRunner
from .common import GPIO, phd_error_msg, rootfs


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


class RebootError(Exception):
    def __init__(self, message):
        msg = Color.colorify("REBOOT ERROR:\n", Color.BOLD)
        msg += str(message) + '\n'
        super().__init__(msg)


class ARMV7M7Runner(DeviceRunner):
    """This class provides interface to run tests on targets with armv7m7 architecture"""

    # redefined by target runners
    SDP = None
    IMAGE = None

    def __init__(self, serial, is_rpi_host=True, log=False):

        # has to be defined before super, because Runner constructor calls set_status, where it's used
        if is_rpi_host:
            self.reset_gpio = GPIO(17, 1)  # JTAG_nSRST, initialize with logic HIGH
            self.power_gpio = GPIO(2, 1)  # connected to relay module, initialize with logic HIGH
            self.boot_gpio = GPIO(4, 0)  # SW7-3 (bootmode pin), initialize with logic LOW
            self.leds = {'red': GPIO(13), 'green': GPIO(18), 'blue': GPIO(12)}

        super().__init__(serial, log, is_rpi_host=is_rpi_host)

        self.logpath = LOG_PATH

        # default values, redefined by specified target runners
        self.phoenixd_port = None
        self.is_cut_power_used = False
        self.flash_memory = 0

    def _restart_by_poweroff(self):
        pass

    def _restart_by_jtag(self):
        self.reset_gpio.low()
        time.sleep(0.050)
        self.reset_gpio.high()

    def rpi_reboot(self, serial_downloader=False, cut_power=False):
        """Reboots a tested device using Raspberry Pi as host-pc."""
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
        phd = None
        try:
            self.reboot(serial_downloader=True, cut_power=self.is_cut_power_used)
            with PloTalker(self.serial_port) as plo:
                if self.logpath:
                    plo.plo.logfile = open(self.logpath, "a")
                Psu(script=self.SDP).run()
                plo.wait_prompt()
                with Phoenixd(self.phoenixd_port) as phd:
                    plo.copy_file2mem(
                        src='usb0',
                        file=self.IMAGE,
                        dst=f'flash{self.flash_memory}',
                    )
            self.reboot(cut_power=self.is_cut_power_used)
        except (TIMEOUT, EOF, PloError, PhoenixdError, RebootError) as exc:
            exception = f'{exc}\n'
            if phd and not isinstance(exc, RebootError):
                exception = phd_error_msg(exception, phd.output())

            logging.info(exception)
            sys.exit(1)

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
                if not test.exec_cmd:
                    # We got plo prompt, we are ready for sending the "go!" command.
                    return True

                with Phoenixd(self.phoenixd_port, dir=load_dir) as phd:
                    plo.app('usb0', test.exec_cmd[0], 'ocram2', 'ocram2')
        except (TIMEOUT, EOF, PloError, PhoenixdError, RebootError) as exc:
            if isinstance(exc, TIMEOUT) or isinstance(exc, EOF):
                test.exception = Color.colorify('EXCEPTION PLO\n', Color.BOLD)
                test.handle_pyexpect_error(plo.plo, exc)
            else:  # RebootError, PloError or PhoenixdError
                test.exception = str(exc)
                test.fail()
                if phd:
                    test.exception = phd_error_msg(test.exception, phd.output())
            return False

        return True

    def set_status(self, status):
        super().set_status(status)
        if self.is_rpi_host:
            if status in self.status_color:
                self.set_led(self.status_color[status])

    def set_led(self, color):
        """Turn on the specified RGB LED's color."""
        if color in self.leds:
            for led_gpio in self.leds.values():
                led_gpio.low()
            self.leds[color].high()
        else:
            print(f'There is no specified led color: {color}')

    def run(self, test):
        if test.skipped():
            return

        if not self.load(test):
            return
        super().run(test)
