#
# Phoenix-RTOS test runner
#
# STM32l4 runner
#
# Copyright 2021 Phoenix Systems
# Authors: Mateusz Niewiadomski
#

import os
import re
import select
import subprocess
import sys
import time

import pexpect.fdpexpect
import serial
from pexpect.exceptions import TIMEOUT, EOF

from .common import DeviceRunner, Color, RebootError, GPIO
from .common import boot_dir
from .common import LOG_PATH, Psu, Phoenixd, PhoenixdError, PloError, PloTalker, DeviceRunner, Runner, RebootError
from .common import GPIO, phd_error_msg, rootfs


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
    """

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

    def __init__(self, target, serial, log=False):
        self.is_rpi_host = True
        self.is_cut_power_used = False # True #False
        if self.is_rpi_host:
            self.reset_gpio = GPIO(17)
            self.reset_gpio.high()
            self.power_gpio = GPIO(2)
            self.power_gpio.high()
        super().__init__(target, serial, log)

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
            # pass
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
        """ Flashing with openocd as a separate process """
        binary_path = os.path.join(boot_dir(self.target), 'phoenix.disk')
        openocd_cmd = [
            'openocd',
            '-f', 'interface/stlink.cfg',
            '-f', 'target/stm32l4x.cfg',
            '-c', "reset_config srst_only srst_nogate connect_assert_srst",
            '-c',  "program {progname} 0x08000000 verify reset exit".format(progname=binary_path)
            ]

        # openocd prints important information (counterintuitively) on stderr, not stdout. However pipe both for user
        openocd_process = subprocess.Popen(
            openocd_cmd,
            stdout=sys.stdout.fileno(),
            stderr=sys.stdout.fileno()
            )

        if openocd_process.wait() != 0:
            print(Color.colorify("OpenOCD error: cannot flash target", Color.BOLD))
            sys.exit(1)

    def load(self, test):
        """Loads test ELF into syspage using plo"""
        # return True
        self.reboot(cut_power=self.is_cut_power_used)
        # phd = None
        # load_dir = str(rootfs(test.target) / 'bin')
        # try:
        #     self.reboot(cut_power=self.is_cut_power_used)
        #     with PloTalker(self.serial_port) as plo:
        #         if self.logpath:
        #             plo.plo.logfile = open(self.logpath, "a")
        #         plo.interrupt_counting()
        #         plo.wait_prompt()
        #         if not test.exec_cmd:
        #             # We got plo prompt, we are ready for sending the "go!" command.
        #             return True

        #         with Phoenixd(self.target, self.phoenixd_port, dir=load_dir) as phd:
        #             plo.app('usb0', test.exec_cmd[0], 'ocram2', 'ocram2')
        # except (TIMEOUT, EOF, PloError, PhoenixdError, RebootError) as exc:
        #     if isinstance(exc, TIMEOUT) or isinstance(exc, EOF):
        #         test.exception = Color.colorify('EXCEPTION PLO\n', Color.BOLD)
        #         test.handle_pyexpect_error(plo.plo, exc)
        #     else:  # RebootError, PloError or PhoenixdError
        #         test.exception = str(exc)
        #         test.fail()
        #         if phd:
        #             test.exception = phd_error_msg(test.exception, phd.output())
        #     return False

        return True

    def run(self, test):
        if test.skipped():
            return

        if not self.load(test):
            return
            
        try:
            self.serial = serial.Serial(self.serial_port, baudrate=self.serial_baudrate)
        except serial.SerialException:
            test.handle_exception()
            return

        # Create pexpect.fdpexpect.fdspawn with modified send() by using oneByOne_fdspawn class
        # codec_errors='ignore' - random light may cause out-of-ascii characters to appear when using optical port
        proc = self.oneByOne_fdspawn(
            self.serial,
            encoding='ascii',
            codec_errors='ignore',
            timeout=test.timeout
            )

        # print('restart board')
        # if sys.stdin in select.select([sys.stdin], [], [], 30)[0]:
        #     sys.stdin.readline()
        # else:
        #     print('It took too long to wait for a key pressing')
        # print('after')
        # proc = pexpect.fdpexpect.fdspawn(self.serial, encoding='utf8', timeout=test.timeout)

        # time.sleep(3)

        # print('1')
        # proc.expect('p')
        # print('2')
        # proc.expect_exact('(psh)% ')
        # print('3')
        if self.logpath:
            proc.logfile = open(self.logpath, "a")

        # FIXME - race on start of Phoenix-RTOS between dummyfs and psh
        # flushing the buffer and sending newline
        # ensures that carret is in newline just after (psh)% prompt
        # flushed = ""
        # while not proc.expect([r'.+', TIMEOUT], timeout=0.1):
        #     flushed += proc.match.group(0)
        # flushed = None
        # proc.send("\n")

        try:
            test.handle(proc)
        finally:
            self.serial.close()
