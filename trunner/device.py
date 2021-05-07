import logging
import os
import signal
import subprocess
import time

import pexpect
import pexpect.fdpexpect
import serial

try:
    import RPi.GPIO
except ImportError:
    print("RPi.GPIO module can't be imported!")

from .config import PHRTOS_PROJECT_DIR, DEVICE_SERIAL


_BOOT_DIR = PHRTOS_PROJECT_DIR / '_boot'

QEMU_CMD = {
    'ia32-generic': (
        'qemu-system-i386',
        [
            '-hda', f'{PHRTOS_PROJECT_DIR}/_boot/phoenix-ia32-generic.disk',
            '-nographic',
            '-monitor', 'none'
        ]
    )
}


def proccess_log_output(proc):
    while True:
        output = proc.stdout.readline().decode('utf-8')
        if proc.poll() is not None and output == '':
            break
        if output:
            logging.info(output)


class Runner:
    """Common interface for test runners"""

    def flash(self):
        """Method used for flashing device with image containing tests."""
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
            test.handle(proc)
        finally:
            self.serial.close()


class GPIO:
    """Wrappee around the RPi.GPIO module. It represents a single OUT pin"""

    def __init__(self, pin):
        self.pin = pin

        RPi.GPIO.setmode(RPi.GPIO.BCM)
        RPi.GPIO.setwarnings(False)
        RPi.GPIO.setup(self.pin, RPi.GPIO.OUT)

    def high(self):
        RPi.GPIO.output(self.pin, RPi.GPIO.HIGH)

    def low(self):
        RPi.GPIO.output(self.pin, RPi.GPIO.LOW)


class IMXRT106xRunner(DeviceRunner):
    """This class provides interface to run test case on IMXRT106x using RaspberryPi.
       GPIO 17 must be connected to the JTAG_nSRST (j21-15) (using an additional resistor 1,5k).
       GPIO 4 must be connected to the SW7-3 (using a resistor 4,3k)."""

    SDP = 'plo-ram-armv7m7-imxrt106x.sdp'
    IMAGE = 'phoenix-armv7m7-imxrt106x.disk'

    def __init__(self, port, phoenixd_port='/dev/ttyUSB0'):
        super().__init__(port)
        self.phoenixd_port = phoenixd_port
        self.reset_gpio = GPIO(17)
        self.reset_gpio.high()
        self.boot_gpio = GPIO(4)

    def reset(self):
        self.reset_gpio.low()
        time.sleep(0.050)
        self.reset_gpio.high()

    def boot(self, serial_downloader=False):
        if serial_downloader:
            self.boot_gpio.low()
        else:
            self.boot_gpio.high()

        self.reset()

    def flash(self):
        self.boot(serial_downloader=True)

        logging.info("psu run!\n")

        psu = subprocess.Popen(
            ['psu', f'{self.SDP}'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=_BOOT_DIR
        )

        proccess_log_output(psu)
        psu.wait()
        if psu.returncode != 0:
            logging.error(f'Command {" ".join(psu.args)} with pid {psu.pid} failed!\n')
            raise Exception('Flashing IMXRT106x failed\n')

        phoenixd = subprocess.Popen([
            'phoenixd',
            '-p', self.phoenixd_port,
            '-b', '115200',
            '-s', '.'],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
            cwd=_BOOT_DIR,
            preexec_fn=os.setpgrp
        )

        logging.info("phoenixd run!\n")

        try:
            target_serial = serial.Serial(self.port, baudrate=115200)
        except serial.SerialException:
            logging.error(f'Port {self.port} not available\n')
            raise

        plo = pexpect.fdpexpect.fdspawn(target_serial)

        try:
            plo.expect_exact("(plo)% ")
            logging.info(f"Copying {self.IMAGE} to target\n")
            plo.send(f"copy com1 {self.IMAGE} flash0 0 0\r\n")
            plo.expect_exact("Finished copying", timeout=60)
            logging.info(f"Finished copying {self.IMAGE} to target\n")
            plo.expect_exact("(plo)% ")
        except pexpect.exceptions.TIMEOUT:
            target_serial.close()
            os.killpg(os.getpgid(phoenixd.pid), signal.SIGTERM)
            raise

        self.boot()

        target_serial.close()
        os.killpg(os.getpgid(phoenixd.pid), signal.SIGTERM)

    def run(self, test):
        self.boot()
        super().run(test)


class QemuRunner(Runner):
    """This class provides interface to run test case using QEMU as a device."""

    def __init__(self, qemu, args):
        self.qemu = qemu
        self.args = args

    def run(self, test):
        if test.skipped():
            return

        proc = pexpect.spawn(self.qemu, args=self.args, encoding='utf-8', timeout=test.timeout)

        try:
            test.handle(proc)
        finally:
            proc.kill(signal.SIGTERM)


class HostRunner(Runner):
    """This class provides interface to run test case using host as a device."""

    def run(self, test):
        if test.skipped():
            return

        test_path = PHRTOS_PROJECT_DIR / f'_boot/{test.target}/{test.exec_bin}'

        try:
            proc = pexpect.spawn(str(test_path), encoding='utf-8', timeout=test.timeout)
        except pexpect.exceptions.ExceptionPexpect:
            test.handle_exception()
            return

        try:
            test.handle(proc, psh=False)
        finally:
            proc.kill(signal.SIGTERM)


class RunnerFactory:
    @staticmethod
    def create(target):
        if target == 'ia32-generic':
            return QemuRunner(*QEMU_CMD[target])
        if target == 'host-pc':
            return HostRunner()
        if target == 'armv7m7-imxrt106x':
            return IMXRT106xRunner(DEVICE_SERIAL)

        raise ValueError(f"Unknown Runner target: {target}")
