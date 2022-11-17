#
# Phoenix-RTOS test runner
#
# STM32l4 runner
#
# Copyright 2021, 2022 Phoenix Systems
# Authors: Mateusz Niewiadomski, Damian Loewnau
#

import select
import sys
import time

import pexpect.fdpexpect

from .common import DeviceRunner, RebootError, GPIO, PloTalker
from .flasher import STM32L4OpenocdFlasher


def hostpc_reboot():
    """Reboots a tested device by a human."""

    print("Please reset the board by pressing `RESET B2` button and press enter")
    print("\tNote that You should press the enter key just after reset button")

    if sys.stdin in select.select([sys.stdin], [], [], 30)[0]:
        sys.stdin.readline()
    else:
        raise RebootError('It took too long to wait for a key pressing')


class STM32L4Runner(DeviceRunner):
    """ Interface to run test cases on STM32L4 target using RaspberryPi.

    The ST-Link programming board should be connected to USB port
    and a target should be connected to ST-Link and powered

    Default configuration for running on Rpi + Nucleo board:

    GPIO 17 must be connected to pin 5 in CN8 header - NRST (using an additional resistor 1,5k)
    GPIO 2 must be connected to an appropriate IN pin used for power cut in relay module
    GPIO 12 must be connected to a blue pin of an RGB LED
    GPIO 13 must be connected to a red pin of an RGB LED
    GPIO 18 must be connected to a green pin of an RGB LED"""

    class oneByOne_fdspawn(pexpect.fdpexpect.fdspawn):
        """ Workaround for Phoenix-RTOS on stm32l4 targets not processing characters fast enough

        Inherits from and is passed to harness instead of pexpect.fdpexpect.fdspawn
        It redefines send() with addition of delay after each byte sent. Delay set to 30ms/byte

        Issue regarding cause of this class existence: github.com/phoenix-rtos/phoenix-rtos-project/issues/235
        """

        def send(self, string):
            ret = 0
            for char in string:
                ret += super().send(char)
                time.sleep(0.03)
            return ret

    def __init__(self, target, serial, is_rpi_host=True, log=False):
        self.is_rpi_host = is_rpi_host
        if self.is_rpi_host:
            self.reset_gpio = GPIO(17)
            self.reset_gpio.high()
            self.power_gpio = GPIO(2)
            self.power_gpio.high()
        # Currently leds are not supported in this runner, that's why is_rpi_host is False
        super().__init__(target, serial, is_rpi_host, log=log)
        self.is_cut_power_used = False

    def _restart_by_jtag(self):
        self.reset_gpio.low()
        time.sleep(0.050)
        self.reset_gpio.high()

    def _restart_by_poweroff(self):
        self.power_gpio.low()
        time.sleep(0.050)
        self.power_gpio.high()
        time.sleep(0.050)

    def rpi_reboot(self, cut_power=False):
        """Reboots a tested device using Raspberry Pi as host-generic-pc."""
        if cut_power:
            self._restart_by_poweroff()
        else:
            self._restart_by_jtag()

    def reboot(self, cut_power=False):
        """Reboots a tested device."""
        if self.is_rpi_host:
            self.rpi_reboot(cut_power)
        else:
            hostpc_reboot()

    def flash(self):
        flasher = STM32L4OpenocdFlasher(self.target)
        flasher.flash()

    def load(self, test):
        self.reboot(cut_power=self.is_cut_power_used)
        # there may be some rubbish on serial right after reboot - to avoid fail we ignore codec errors
        with PloTalker(self.serial_port, codec_errors='ignore', replaced_fdspawn=self.oneByOne_fdspawn) as plo:
            if self.logpath:
                plo.plo.logfile = open(self.logpath, "a")
            plo.interrupt_counting()
            plo.wait_prompt()
            if not test.exec_cmd and not test.syspage_prog:
                return True
        return True

    def run(self, test):
        if test.skipped():
            return

        if not self.load(test):
            return
        super().run(test, replaced_fdspawn=self.oneByOne_fdspawn)
