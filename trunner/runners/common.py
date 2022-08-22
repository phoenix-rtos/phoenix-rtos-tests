#
# Phoenix-RTOS test runner
#
# Common parts of phoenix-rtos test runners
#
# Copyright 2021, 2022 Phoenix Systems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import importlib
import logging
import os

import sys
import time

from abc import ABC, abstractmethod
from pathlib import Path

import subprocess
import pexpect
import pexpect.fdpexpect
import serial

from trunner.config import PHRTOS_PROJECT_DIR
from trunner.tools.color import Color

LOG_PATH = '/tmp/phoenix_test.log'
LOG_PATH_PHOENIXD = '/tmp/phoenix_test_phoenixd.log'


def boot_dir(target: str) -> Path:
    return PHRTOS_PROJECT_DIR / '_boot' / target


def rootfs(target: str) -> Path:
    return PHRTOS_PROJECT_DIR / '_fs' / target / 'root'


def wait_for_dev(port, timeout=0):
    asleep = 0

    # naive wait for dev
    while not os.path.exists(port):
        time.sleep(0.01)
        asleep += 0.01
        if timeout and asleep >= timeout:
            raise TimeoutError


def power_usb_ports(enable: bool):
    uhubctl = subprocess.run([
        'uhubctl',
        '-l', '2',
        '-a', f'{1 if enable else 0}'],
        stdout=subprocess.DEVNULL
    )
    if uhubctl.returncode != 0:
        logging.error('uhubctl failed!\n')
        raise Exception('RPi usb ports powering up/down failed!')


def unbind_rpi_usb(port_address):
    try:
        with open('/sys/bus/usb/drivers/usb/unbind', 'w') as file:
            file.write(port_address)
    except PermissionError:
        logging.error("/sys/bus/usb/drivers/usb/unbind: PermissionError\n\
        If You launch test runner locally:\n\
        Add 'sudo chmod a+w /sys/bus/usb/drivers/usb/unbind' to /etc/rc.local\n\
        If You use Docker:\n\
        Set the appropriate permissions\n")
        sys.exit(1)


class RebootError(Exception):
    def __init__(self, message):
        msg = Color.colorify("REBOOT ERROR:\n", Color.BOLD)
        msg += str(message) + '\n'
        super().__init__(msg)


class PloError(Exception):
    def __init__(self, message, expected, cmd):
        if not message:
            message = "[no response]"
        msg = f'{Color.BOLD}PLO ERROR:{Color.END} {message}\n' \
              f'{Color.BOLD}CMD PASSED:{Color.END} {cmd}\n'
        if expected:
            msg += f'{Color.BOLD}EXPECTED:{Color.END} {str(expected)}\n'

        super().__init__(msg)


class PloTalker:
    """Interface to communicate with plo"""

    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        self.plo = None

    @classmethod
    def from_pexpect(cls, pexpect_fd):
        """ PloTalker can be created by passing pexpect spawn object directly.
            User should handle port and process by himself. """

        obj = cls(port=None)
        obj.plo = pexpect_fd
        return obj

    def interrupt_counting(self):
        """ Interrupts timer counting to enter plo """
        self.plo.expect_exact('Waiting for input', timeout=3)
        self.plo.send('\r\n')

    def open(self):
        try:
            self.serial = serial.Serial(self.port, baudrate=self.baudrate)
        except serial.SerialException:
            logging.error(f'Port {self.port} not available\n')
            raise

        try:
            self.plo = pexpect.fdpexpect.fdspawn(self.serial, encoding='ascii', timeout=8)
        except Exception:
            self.serial.close()
            raise

        return self

    def close(self):
        self.serial.close()

    def __enter__(self):
        return self.open()

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def wait_prompt(self, timeout=8):
        self.plo.expect_exact("(plo)% ", timeout=timeout)

    def cmd(self, cmd, timeout=8):
        self.plo.send(cmd + '\r\n')
        # Wait for an eoched command
        self.plo.expect_exact(cmd)
        try:
            self.plo.expect_exact('(plo)%', timeout=timeout)
            # red color means that, there was some error in plo
            if "\x1b[31m" in self.plo.before:
                raise PloError(self.plo.before, expected="(plo)% ", cmd=cmd)
        except pexpect.TIMEOUT:
            raise PloError(self.plo.before, expected="(plo)% ", cmd=cmd)

    def app(self, device, file, imap, dmap, exec=False):
        exec = '-x' if exec else ''
        self.cmd(f'app {device} {exec} {file} {imap} {dmap}', timeout=30)

    def copy(self, src, src_obj, dst, dst_obj, src_size='', dst_size=''):
        logging.info('Copying the system image, please wait...\n')
        self.cmd(f'copy {src} {src_obj} {src_size} {dst} {dst_obj} {dst_size}', timeout=140)

    def copy_file2mem(self, src, file, dst='flash1', off=0, size=0):
        self.copy(
            src=src,
            src_obj=file,
            dst=dst,
            dst_obj=off,
            dst_size=size
        )

    def go(self):
        self.plo.send('go!\r')


class Runner(ABC):
    """Common interface for test runners"""

    BUSY = 'BUSY'
    SUCCESS = 'SUCCESS'
    FAIL = 'FAIL'

    def __init__(self, target, log=False):
        # Busy status is set from the start to the end of the specified runner's run
        self.status = Runner.BUSY
        self.set_status(self.status)
        self.target = target
        if log:
            if os.path.exists(LOG_PATH):
                os.remove(LOG_PATH)
            self.logpath = LOG_PATH
        else:
            self.logpath = None

    def set_status(self, status):
        """Method for sygnalising a current runner status: busy/failed/succeeded"""
        # for now, not used in all target runners
        self.status = status

    @abstractmethod
    def flash(self):
        """Method used for flashing a device with the image containing tests."""
        pass

    @abstractmethod
    def run(self, test):
        """Method used for running a single test case which is represented by TestCase class."""
        pass


class DeviceRunner(Runner):
    """This class provides interface to run tests on hardware targets using serial port"""

    status_color = {Runner.BUSY: 'blue', Runner.SUCCESS: 'green', Runner.FAIL: 'red'}

    def __init__(self, target, serial, is_rpi_host, log=False):
        if is_rpi_host:
            self.leds = {'red': GPIO(13), 'green': GPIO(18), 'blue': GPIO(12)}
        super().__init__(target, log)
        self.serial_port = serial[0]
        self.serial_baudrate = serial[1]
        self.serial = None

    def run(self, test, send_go=True):
        if test.skipped():
            return

        try:
            self.serial = serial.Serial(self.serial_port, baudrate=self.serial_baudrate)
        except serial.SerialException:
            test.handle_exception()
            return

        proc = pexpect.fdpexpect.fdspawn(self.serial, encoding='utf-8', timeout=test.timeout)
        if self.logpath:
            proc.logfile = open(self.logpath, "a")

        try:
            if send_go:
                PloTalker.from_pexpect(proc).go()
            test.handle(proc)
        finally:
            self.serial.close()
            if os.path.exists(LOG_PATH_PHOENIXD):
                os.remove(LOG_PATH_PHOENIXD)

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


class GPIO:
    """Wrapper around the RPi.GPIO module. It represents a single OUT pin"""

    def __init__(self, pin, init=0):
        self.pin = pin
        self.gpio = importlib.import_module('RPi.GPIO')

        self.gpio.setmode(self.gpio.BCM)
        self.gpio.setwarnings(False)
        if init == 0:
            self.gpio.setup(self.pin, self.gpio.OUT, initial=self.gpio.LOW)
        else:
            self.gpio.setup(self.pin, self.gpio.OUT, initial=self.gpio.HIGH)

    def high(self):
        self.gpio.output(self.pin, self.gpio.HIGH)

    def low(self):
        self.gpio.output(self.pin, self.gpio.LOW)
