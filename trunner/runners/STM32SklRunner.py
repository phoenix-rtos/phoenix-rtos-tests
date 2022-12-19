#
# Phoenix-RTOS test runner
#
# STM32l4 runner
#
# Copyright 2021, 2022 Phoenix Systems
# Authors: Mateusz Niewiadomski, Damian Loewnau
#

import time

import pexpect.fdpexpect

from .common import DeviceRunner, GPIO
from .flasher import STM32L4OpenocdFlasher


class STM32SklRunner(DeviceRunner):
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
            self.ledr_gpio = GPIO(22)
            self.ledg_gpio = GPIO(23)
            self.ledb_gpio = GPIO(27)
        super().__init__(target, serial, is_rpi_host=True, log=log)

    def flash(self):
        flasher = STM32L4OpenocdFlasher(self.target)
        flasher.flash()

    def load(self, test):
        pass

    def run(self, test):
        if test.skipped():
            return

        super().run(test, replaced_fdspawn=self.oneByOne_fdspawn)
