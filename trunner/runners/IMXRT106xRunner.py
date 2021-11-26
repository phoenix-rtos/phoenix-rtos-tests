#
# Phoenix-RTOS test runner
#
# IMXRT1064 runner
#
# Copyright 2021 Phoenix SYstems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import time
import sys
from pexpect.exceptions import TIMEOUT, EOF

from .common import DeviceRunner, PloTalker, PloError, Psu, Phoenixd, PhoenixdError, GPIO, Color, logging
from .common import unbind_rpi_usb, power_usb_ports, wait_for_dev, phd_error_msg, rootfs


class IMXRT106xRunner(DeviceRunner):
    """This class provides interface to run test case on IMXRT106x using RaspberryPi.
       GPIO 17 must be connected to the JTAG_nSRST (j21-15) (using an additional resistor 1,5k).
       GPIO 4 must be connected to the SW7-3 (using a resistor 4,3k).
       GPIO 2 must be connected to an appropriate IN pin in relay module"""

    SDP = 'plo-ram-armv7m7-imxrt106x.sdp'
    IMAGE = 'phoenix-armv7m7-imxrt106x.disk'

    def __init__(
        self,
        port,
        phoenixd_port='/dev/serial/by-id/usb-Phoenix_Systems_plo_CDC_ACM-if00',
        is_cut_power_used=False
    ):
        super().__init__(port[0])
        self.port_usb = port[1]
        self.phoenixd_port = phoenixd_port
        self.is_cut_power_used = is_cut_power_used
        self.reset_gpio = GPIO(17)
        self.reset_gpio.high()
        self.power_gpio = GPIO(2)
        self.power_gpio.high()
        self.boot_gpio = GPIO(4)
        self.ledr_gpio = GPIO(13)
        self.ledg_gpio = GPIO(18)
        self.ledb_gpio = GPIO(12)
        self.ledb_gpio.high()

    def led(self, color, state="on"):
        if state == "on" or state == "off":
            self.ledr_gpio.low()
            self.ledg_gpio.low()
            self.ledb_gpio.low()
        if state == "on":
            if color == "red":
                self.ledr_gpio.high()
            if color == "green":
                self.ledg_gpio.high()
            if color == "blue":
                self.ledb_gpio.high()

    def _restart_by_jtag(self):
        self.reset_gpio.low()
        time.sleep(0.050)
        self.reset_gpio.high()

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

    def reboot(self, serial_downloader=False, cut_power=False):
        if serial_downloader:
            self.boot_gpio.low()
        else:
            self.boot_gpio.high()

        if cut_power:
            self._restart_by_poweroff()
        else:
            self._restart_by_jtag()

    def flash(self):
        self.reboot(serial_downloader=True, cut_power=self.is_cut_power_used)

        phd = None
        try:
            with PloTalker(self.serial_port) as plo:
                Psu(script=self.SDP).run()
                plo.wait_prompt()
                with Phoenixd(self.phoenixd_port) as phd:
                    plo.copy_file2mem(
                        src='usb0',
                        file=self.IMAGE,
                        dst='flash1',
                        off=0
                    )
        except (TIMEOUT, EOF, PloError, PhoenixdError) as exc:
            exception = f'{exc}\n'
            if phd:
                exception = phd_error_msg(exception, phd.output())

            logging.info(exception)
            sys.exit(1)

        self.reboot(cut_power=self.is_cut_power_used)

    def load(self, test):
        """Loads test ELF into syspage using plo"""

        phd = None
        load_dir = str(rootfs(test.target) / 'bin')
        self.reboot(cut_power=self.is_cut_power_used)
        try:
            with PloTalker(self.serial_port) as plo:
                # Because of powering all Rpi ports after powering the board,
                # there is need to second reboot (without cut power) in order to capture all data
                # (when using _restart_by_poweroff)
                if self.is_cut_power_used:
                    self.reboot(cut_power=False)
                plo.wait_prompt()

                if not test.exec_cmd:
                    # We got plo prompt, we are ready for sending the "go!" command.
                    return True

                with Phoenixd(self.phoenixd_port, dir=load_dir) as phd:
                    plo.app('usb0', test.exec_cmd[0], 'ocram2', 'ocram2')
        except (TIMEOUT, EOF, PloError, PhoenixdError) as exc:
            if isinstance(exc, PloError) or isinstance(exc, PhoenixdError):
                test.exception = str(exc)
                test.fail()
            else:  # TIMEOUT or EOF
                test.exception = Color.colorify('EXCEPTION PLO\n', Color.BOLD)
                test.handle_pyexpect_error(plo.plo, exc)

            if phd:
                test.exception = phd_error_msg(test.exception, phd.output())

            return False

        return True

    def run(self, test):
        if test.skipped():
            return

        if not self.load(test):
            return

        super().run(test)
