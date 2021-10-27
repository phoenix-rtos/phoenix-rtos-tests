#
# Phoenix-RTOS test runner
#
# Common parts of phoenix-rtos test runners
#
# Copyright 2021 Phoenix SYstems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import importlib
import logging
import os
import signal
import sys
import threading
import time

import pexpect
import pexpect.fdpexpect
import serial
import subprocess

from pathlib import Path

from trunner.config import PHRTOS_PROJECT_DIR
from trunner.tools.color import Color


_BOOT_DIR = PHRTOS_PROJECT_DIR / '_boot'


def rootfs(target: str) -> Path:
    return PHRTOS_PROJECT_DIR / '_fs' / target / 'root'


def is_github_actions():
    return os.getenv('GITHUB_ACTIONS', False)


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


class Psu:
    """Wrapper for psu program"""

    def __init__(self, script, cwd=_BOOT_DIR):
        self.script = script
        self.cwd = cwd
        self.proc = None

    def read_output(self):
        if is_github_actions():
            logging.info('::group::Run psu\n')

        while True:
            line = self.proc.readline()
            if not line:
                break

            logging.info(line)

        if is_github_actions():
            logging.info('::endgroup::\n')

    def run(self):
        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            'psu',
            [f'{self.script}'],
            cwd=self.cwd,
            encoding='utf-8'
        )

        self.read_output()
        self.proc.wait()
        if self.proc.exitstatus != 0:
            logging.error('psu failed!\n')
            raise Exception('Flashing IMXRT106x failed!')


def phd_error_msg(message, output):
    msg = message
    msg += Color.colorify('\nPHOENIXD OUTPUT:\n', Color.BOLD)
    msg += output

    return msg


class PhoenixdError(Exception):
    pass


class Phoenixd:
    """ Wrapper for phoenixd program"""

    def __init__(
        self,
        port,
        baudrate=460800,
        dir='.',
        cwd=_BOOT_DIR,
        wait_dispatcher=True
    ):
        self.port = port
        self.baudrate = baudrate
        self.dir = dir
        self.cwd = cwd
        self.proc = None
        self.reader_thread = None
        self.wait_dispatcher = wait_dispatcher
        self.dispatcher_event = None
        self.output_buffer = ''

    def _reader(self):
        """ This method is intended to be run as a separated thread. It reads output of proc
            line by line and saves it in the output_buffer. Additionally, if wait_dispatcher is true,
            it searches for a line stating that message dispatcher has started """

        while True:
            line = self.proc.readline()
            if not line:
                break

            if self.wait_dispatcher and not self.dispatcher_event.is_set():
                msg = f'Starting message dispatcher on [{self.port}] (speed={self.baudrate})'
                if msg in line:
                    self.dispatcher_event.set()

            self.output_buffer += line

    def run(self):
        try:
            wait_for_dev(self.port, timeout=10)
        except TimeoutError as exc:
            raise PhoenixdError(f'couldn\'t find {self.port}') from exc

        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            'phoenixd',
            ['-p', self.port,
             '-b', str(self.baudrate),
             '-s', self.dir],
            cwd=self.cwd,
            encoding='utf-8'
        )

        self.dispatcher_event = threading.Event()
        self.reader_thread = threading.Thread(target=self._reader)
        self.reader_thread.start()

        if self.wait_dispatcher:
            # Reader thread will notify us that message dispatcher has just started
            dispatcher_ready = self.dispatcher_event.wait(timeout=5)
            if not dispatcher_ready:
                self.kill()
                msg = 'message dispatcher did not start!'
                raise PhoenixdError(msg)

        return self.proc

    def output(self):
        output = self.output_buffer
        if is_github_actions():
            output = '::group::phoenixd output\n' + output + '\n::endgroup::\n'

        return output

    def kill(self):
        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)
        self.reader_thread.join(timeout=10)
        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)

    def __enter__(self):
        self.run()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.kill()


class PloError(Exception):
    def __init__(self, message, expected):
        msg = Color.colorify("PLO ERROR:\n", Color.BOLD)
        msg += str(message) + '\n'
        if expected:
            msg += Color.colorify("EXPECTED:\n", Color.BOLD)
            msg += str(expected) + '\n'

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

    def open(self):
        try:
            self.serial = serial.Serial(self.port, baudrate=self.baudrate)
        except serial.SerialException:
            logging.error(f'Port {self.port} not available\n')
            raise

        try:
            self.plo = pexpect.fdpexpect.fdspawn(self.serial, timeout=8)
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

    def expect_prompt(self, timeout=8):
        idx = self.plo.expect([r"\(plo\)% ", r"(.*?)\n"], timeout=timeout)
        if idx == 1:
            # Something else than prompt was printed, raise error
            line = self.plo.match.group(0)
            raise PloError(line, expected="(plo)% ")

    def cmd(self, cmd, timeout=8):
        self.plo.send(cmd + '\r\n')
        # Wait for an eoched command
        self.plo.expect_exact(cmd)
        # There might be some ASCII escape characters, we wait only for a new line
        self.plo.expect_exact('\n', timeout=timeout)

    def app(self, device, file, imap, dmap, exec=False):
        exec = '-x' if exec else ''
        self.cmd(f'app {device} {exec} {file} {imap} {dmap}', timeout=30)
        self.expect_prompt()

    def copy(self, src, src_obj, dst, dst_obj, src_size='', dst_size=''):
        self.cmd(f'copy {src} {src_obj} {src_size} {dst} {dst_obj} {dst_size}', timeout=60)
        self.expect_prompt()

    def copy_file2mem(self, src, file, dst='flash1', off=0, size=0):
        self.copy(
            src=src,
            src_obj=file,
            dst=dst,
            dst_obj=off,
            dst_size=size
        )

    def go(self):
        self.plo.send('go!\r\n')


class Runner:
    """Common interface for test runners"""

    def flash(self):
        """Method used for flashing a device with the image containing tests."""
        pass

    def run(self, test):
        """Method used for running a single test case which is represented by TestCase class."""
        pass


class DeviceRunner(Runner):
    """This class provides interface to run test case using serial port"""

    def __init__(self, port):
        self.port = port
        self.serial = None

    def run(self, test):
        if test.skipped():
            return

        try:
            self.serial = serial.Serial(self.port, baudrate=115200)
        except serial.SerialException:
            test.handle_exception()
            return

        proc = pexpect.fdpexpect.fdspawn(self.serial, encoding='utf-8', timeout=test.timeout)

        try:
            PloTalker.from_pexpect(proc).go()
            test.handle(proc)
        finally:
            self.serial.close()


class GPIO:
    """Wrapper around the RPi.GPIO module. It represents a single OUT pin"""

    def __init__(self, pin):
        self.pin = pin
        self.gpio = importlib.import_module('RPi.GPIO')

        self.gpio.setmode(self.gpio.BCM)
        self.gpio.setwarnings(False)
        self.gpio.setup(self.pin, self.gpio.OUT, initial=self.gpio.LOW)

    def high(self):
        self.gpio.output(self.pin, self.gpio.HIGH)

    def low(self):
        self.gpio.output(self.pin, self.gpio.LOW)
