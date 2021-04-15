import signal

import pexpect
import pexpect.fdpexpect
import serial

from .config import PHRTOS_PROJECT_DIR


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


class DeviceRunner:
    """This class provides interface to run test case using serial port"""

    def __init__(self, port):
        self.port = port
        self.serial = None

    def run(self, test):
        if test.skipped():
            return

        try:
            self.serial = serial.Serial(self.port, baudrate=115200)
        except serial.SerialException as exc:
            raise exc

        proc = pexpect.fdpexpect.fdspawn(self.serial, encoding='utf-8', timeout=test.timeout)

        try:
            test.handle(proc)
        finally:
            self.serial.close()


class QemuRunner:
    """This class provides interface to run test case using QEMU as a device."""

    def __init__(self, qemu, args):
        self.qemu = qemu
        self.args = args

    def run(self, test):
        if test.skipped():
            return

        proc = pexpect.spawn(self.qemu, args=self.args, encoding='utf-8', timeout=test.timeout)

        res = False
        try:
            res = test.handle(proc)
        finally:
            if not res:
                proc.kill(signal.SIGKILL)
            else:
                proc.kill(signal.SIGTERM)
