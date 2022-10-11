#
# Phoenix-RTOS test runner
#
# ZYNQ7000Zedboard runner
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau

import select
import sys
import time

from .common import DeviceRunner, GPIO, RebootError
from .flasher import ZYNQ7000JtagFlasher


def hostpc_reboot(jtag_mode=False):
    """Reboots a tested device by a human."""
    if jtag_mode:
        print("\n\tPlease change the board's boot mode to jtag, reset the board and press enter")
    else:
        print("\n\tPlease change the board's boot mode to Quad SPI flash if the current mode is different.\n\
        Next, reset the board and press enter")
    print("\tNote that You should press the enter key just after reset button")

    if sys.stdin in select.select([sys.stdin], [], [], 30)[0]:
        sys.stdin.readline()
    else:
        raise RebootError('It took too long to wait for a key pressing')


class ZYNQ7000ZedboardRunner(DeviceRunner):
    """This class provides interface to run test case on ZYNQ7000Zedboard using RaspberryPi.

    Default configuration for running on Rpi:

    GPIO 4 must be connected to an appropriate IN pin used for boot mode change in relay module
    GPIO 2 must be connected to an appropriate IN pin used for power cut in relay module
    GPIO 12 must be connected to a blue pin of an RGB LED
    GPIO 13 must be connected to a red pin of an RGB LED
    GPIO 18 must be connected to a green pin of an RGB LED"""

    def __init__(self, target, port, phoenixd_port, is_rpi_host=True, log=False):
        self.is_rpi_host = is_rpi_host
        if self.is_rpi_host:
            self.power_gpio = GPIO(2)
            self.power_gpio.high()
            self.boot_gpio = GPIO(4)

        super().__init__(target, port, is_rpi_host, log)
        self.phoenixd_port = phoenixd_port
        # Hardware test unit does not support jtag reset
        self.is_cut_power_used = True
        self.flash_bank = 0
        # We don't enter plo, because we don't need to load tests (they are the part of the root fs)
        self.send_go = False

    def _restart_by_poweroff(self):
        self.power_gpio.low()
        # optimal power off time to prevent sustaining chips, e.g. flash memory, related to #540 issue
        time.sleep(0.750)
        self.power_gpio.high()
        time.sleep(0.050)

    def rpi_reboot(self, jtag_mode=False):
        """Reboots a tested device using Raspberry Pi as host-generic-pc."""
        if jtag_mode:
            self.boot_gpio.low()
        else:
            self.boot_gpio.high()

        self._restart_by_poweroff()

    def reboot(self, flashing_mode=False):
        """Reboots a tested device."""
        if self.is_rpi_host:
            self.rpi_reboot(jtag_mode=flashing_mode)
        else:
            hostpc_reboot(jtag_mode=flashing_mode)

    def flash(self):
        flasher = ZYNQ7000JtagFlasher(
            self.target,
            self.serial_port,
            self.phoenixd_port,
            self.flash_bank,
            self.logpath
        )

        flasher.flash(self.reboot)

    def run(self, test):
        if test.skipped():
            return

        self.reboot()
        super().run(test)
