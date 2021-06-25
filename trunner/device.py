import logging
import os
import signal
import subprocess
import sys
import threading
import time

import pexpect
import pexpect.fdpexpect
import serial

try:
    import RPi.GPIO
except (ImportError, RuntimeError):
    pass

from .config import PHRTOS_PROJECT_DIR, DEVICE_SERIAL
from .tools.color import Color


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


def is_github_actions():
    return os.getenv('GITHUB_ACTIONS', False)


class Psu:
    """Wrapper for psu program"""

    def __init__(self, script, cwd=_BOOT_DIR):
        self.script = script
        self.cwd = cwd
        self.proc = None

    def read_output(self):
        if is_github_actions():
            logging.info('::group::psu\n')

        while True:
            line = self.proc.stdout.readline().decode('utf-8')
            if not line:
                break

            logging.info(line)

        if is_github_actions():
            logging.info('::endgroup::\n')

    def run(self):
        self.proc = subprocess.Popen(
            ['psu', f'{self.script}'],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            cwd=self.cwd
        )

        self.read_output()
        self.proc.wait()
        if self.proc.returncode != 0:
            logging.error(f'Command {" ".join(self.proc.args)} with pid {self.proc.pid} failed!\n')
            raise Exception('Flashing IMXRT106x failed')


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
            dispatcher_ready = self.dispatcher_event.wait(timeout=10)
            if not dispatcher_ready:
                self.kill()
                msg = f'message dispatcher did not start! Phoenixd output:\n{self.output()}'
                raise PhoenixdError(msg)

        return self.proc

    def output(self):
        output = self.output_buffer
        if is_github_actions():
            output = '::group::Pheonixd\n' + output + '::endgroup::\n'

        return output

    def kill(self):
        os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)
        self.reader_thread.join(timeout=10)
        if self.reader_thread.is_alive():
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
            PloTalker.from_pexpect(proc).go()
            test.handle(proc)
        finally:
            self.serial.close()


class GPIO:
    """Wrapper around the RPi.GPIO module. It represents a single OUT pin"""

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

    def __init__(
        self,
        port,
        phoenixd_port='/dev/serial/by-id/usb-Phoenix_Systems_plo_CDC_ACM-if00'
    ):
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

        Psu(script=self.SDP).run()

        phd = None
        try:
            with PloTalker(self.port) as plo:
                plo.wait_prompt()
                with Phoenixd(self.phoenixd_port) as phd:
                    plo.copy_file2mem(
                        src='usb0',
                        file=self.IMAGE,
                        dst='flash1',
                        off=0
                    )
        except (pexpect.exceptions.TIMEOUT, pexpect.exceptions.EOF, PloError) as exc:
            exception = f'{exc}\n'
            if phd:
                exception += Color.colorify('\nPHOENIXD OUTPUT:\n', Color.BOLD)
                exception += phd.output()
            logging.info(exception)
            sys.exit(1)

        self.boot()

    def load(self, test):
        """Loads test ELF into syspage using plo"""

        phd = None
        load_dir = 'test/armv7m7-imxrt106x'
        try:
            with PloTalker(self.port) as plo:
                self.boot()
                plo.wait_prompt()
                with Phoenixd(self.phoenixd_port, dir=load_dir) as phd:
                    plo.app('usb0', test.exec_bin, 'ocram2', 'ocram2')
        except (pexpect.exceptions.TIMEOUT, pexpect.exceptions.EOF, PloError) as exc:
            if isinstance(exc, PloError):
                test.exception = str(exc)
                test.fail()
            else:  # TIMEOUT or EOF
                test.exception = Color.colorify('EXCEPTION PLO\n', Color.BOLD)
                test.handle_pyexpect_error(plo.plo, exc)

            if phd:
                test.exception += Color.colorify('\nPHOENIXD OUTPUT:\n', Color.BOLD)
                test.exception += phd.output()

            return False

        return True

    def run(self, test):
        if test.skipped():
            return

        if not self.load(test):
            return

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
