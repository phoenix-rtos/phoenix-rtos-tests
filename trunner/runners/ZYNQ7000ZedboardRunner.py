#
# Phoenix-RTOS test runner
#
# ZYNQ7000Zedboard runner
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau

from .ARMV7M7Runner import ARMV7M7Runner

import time


class ZYNQ7000ZedboardRunner(ARMV7M7Runner):
    """This class provides interface to run test case on ZYNQ7000Zedboard using RaspberryPi.

    Default configuration for running on Rpi:

    GPIO 4 must be connected to an appropriate IN pin used for boot mode change in relay module
    GPIO 2 must be connected to an appropriate IN pin used for power cut in relay module
    GPIO 12 must be connected to a blue pin of an RGB LED
    GPIO 13 must be connected to a red pin of an RGB LED
    GPIO 18 must be connected to a green pin of an RGB LED"""

    IMAGE = 'phoenix-armv7a9-zynq7000-zedboard.disk'

    def __init__(self, port, phoenixd_port, is_rpi_host=True, log=False):
        # output right after opening serial port may be noisy on this target
        # serial port disappears after powering down a destination board
        super().__init__(
                        port,
                        is_rpi_host,
                        log, disconnecting_serial=True,
                        noisy=True,
                        run_psu=False,
                        dest_code='ddr',
                        dest_exec='ddr')
        self.phoenixd_port = phoenixd_port
        # Hardware test unit does not support jtag reset
        self.is_cut_power_used = True
        self.flash_memory = 0

    def _restart_by_poweroff(self):
        self.power_gpio.low()
        time.sleep(0.350)
        self.power_gpio.high()
        time.sleep(0.350)
