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
from .common import LOG_PATH, Psu, Phoenixd, PhoenixdError, PloError, PloTalker, DeviceRunner, Runner
from .common import GPIO, phd_error_msg, rootfs, wait_for_dev


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

    status_color = {Runner.BUSY: 'blue', Runner.SUCCESS: 'green', Runner.FAIL: 'red'}

    # redefined by target runners
    SDP = None
    IMAGE = None

    def __init__(
        self,
        target,
        serial,
        is_rpi_host=True,
        log=False,
        disconnecting_serial=False,
        noisy=False,
        run_psu=True,
        dest_code='ocram2',
        dest_exec='ocram2'
    ):
        # has to be defined before super, because Runner constructor calls set_status, where it's used
        self.is_rpi_host = is_rpi_host
        if self.is_rpi_host:
            self.reset_gpio = GPIO(17)
            self.reset_gpio.high()
            self.power_gpio = GPIO(2)
            self.power_gpio.high()
            self.boot_gpio = GPIO(4)
            self.leds = {'red': GPIO(13), 'green': GPIO(18), 'blue': GPIO(12)}
            self.logpath = LOG_PATH
        super().__init__(target, serial, log)
        self.disconnecting_serial = disconnecting_serial
        self.noisy = noisy
        self.run_psu = run_psu
        # used for loading test binaries - destination memory for storing test code and execution
        self.dest_code = dest_code
        self.dest_exec = dest_exec
        self.codec_errors = 'strict'
        self.catch_waiting_statement = True
        if self.noisy:
            self.codec_errors = 'ignore'
            self.catch_waiting_statement = False
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
        phd = None
        try:
            if self.run_psu:
                self.reboot(serial_downloader=True, cut_power=self.is_cut_power_used)
            else:
                # If plo isn't uploaded using psu, we will need to enter it from flash boot mode
                self.reboot(serial_downloader=False, cut_power=self.is_cut_power_used)
            if self.disconnecting_serial:
                wait_for_dev(self.serial_port, timeout=8, wait_for_disappear=True)
            with PloTalker(self.serial_port, codec_errors=self.codec_errors) as plo:
                if self.logpath:
                    plo.plo.logfile = open(self.logpath, "a")
                if self.run_psu:
                    Psu(self.target, script=self.SDP).run()
                # after connecting back to serial - sending newline may be needed to see the prompt
                elif self.disconnecting_serial:
                    plo.interrupt_counting(self.catch_waiting_statement)
                plo.wait_prompt()
                with Phoenixd(self.target, self.phoenixd_port) as phd:
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
            if self.disconnecting_serial:
                # The serial port has to be available within ~2s to interrupt counting and enter plo
                wait_for_dev(self.serial_port, timeout=1.9, wait_for_disappear=True)
            with PloTalker(self.serial_port, codec_errors=self.codec_errors) as plo:
                if self.logpath:
                    plo.plo.logfile = open(self.logpath, "a")
                plo.interrupt_counting(self.catch_waiting_statement)
                plo.wait_prompt()
                if not test.exec_cmd and not test.syspage_prog:
                    # We got plo prompt, we are ready for sending the "go!" command.
                    return True

                with Phoenixd(self.target, self.phoenixd_port, dir=load_dir) as phd:
                    prog = test.exec_cmd[0] if test.exec_cmd else test.syspage_prog
                    plo.app('usb0', prog, self.dest_code, self.dest_exec)
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
