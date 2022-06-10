#
# Phoenix-RTOS test runner
#
# IMXRT117x runner
#
# Copyright 2021 Phoenix Systems
# Authors: Damian Loewnau, Jakub Sarzyński
#

import time

from .ARMV7M7Runner import ARMV7M7Runner


class IMXRT117xRunner(ARMV7M7Runner):
    """This class provides interface to run test case on IMXRT117x using RaspberryPi.

    Default configuration for running on Rpi:

    GPIO 17 must be connected to the JTAG_nSRST (j21-15) (using an additional resistor 1,5k).
    GPIO 4 must be connected to the pin 4 in P1 Header (using a resistor 3,3k).
    GPIO 2 must be connected to an appropriate IN pin in relay module
    GPIO 12 must be connected to a blue pin of an RGB LED
    GPIO 13 must be connected to a red pin of an RGB LED
    GPIO 18 must be connected to a green pin of an RGB LED"""

    SDP = 'plo-ram.sdp'
    IMAGE = 'phoenix.disk'

    def __init__(self, target, port, phoenixd_port, is_cut_power_used=False, is_rpi_host=True, log=False):
        super().__init__(target, port, is_rpi_host, log)
        self.phoenixd_port = phoenixd_port
        self.is_cut_power_used = is_cut_power_used
        self.flash_memory = 0

    def _restart_by_poweroff(self):
        self.power_gpio.low()
        time.sleep(0.500)
        self.power_gpio.high()
        time.sleep(0.500)
