#
# Phoenix-RTOS test runner
#
# IMXRT1064 runner
#
# Copyright 2021 Phoenix SYstems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import logging
import time
import sys

from .ARMV7M7Runner import ARMV7M7Runner
from .common import unbind_rpi_usb, power_usb_ports, wait_for_dev


class IMXRT106xRunner(ARMV7M7Runner):
    """This class provides interface to run test case on IMXRT106x using RaspberryPi.

    Default configuration for running on Rpi:

    GPIO 17 must be connected to the JTAG_nSRST (j21-15) (using an additional resistor 1,5k).
    GPIO 4 must be connected to the SW7-3 (using a resistor 4,3k).
    GPIO 2 must be connected to an appropriate IN pin in relay module
    GPIO 12 must be connected to a blue pin of an RGB LED
    GPIO 13 must be connected to a red pin of an RGB LED
    GPIO 18 must be connected to a green pin of an RGB LED"""

    SDP = 'plo-ram.sdp'
    IMAGE = 'phoenix.disk'

    def __init__(self, target, port, phoenixd_port, is_rpi_host=True, log=False):
        super().__init__(target, port[0], is_rpi_host, log)
        self.port_usb = port[1]
        self.phoenixd_port = phoenixd_port
        # restart by power off is temporarily disabled for this target
        self.is_cut_power_used = False
        self.flash_memory = 1

    def _restart_by_poweroff(self):
        unbind_rpi_usb(self.port_usb)

        power_usb_ports(False)
        self.power_gpio.low()
        time.sleep(0.500)
        self.power_gpio.high()
        time.sleep(0.500)
        power_usb_ports(True)

        try:
            wait_for_dev(self.port_usb, timeout=30)
        except TimeoutError:
            logging.error('Serial port not found!\n')
            sys.exit(1)
