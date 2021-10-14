#
# Phoenix-RTOS test runner
#
# ia32-generic qemu emulator runner
#
# Copyright 2021 Phoenix SYstems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import pexpect
import signal

from .common import Runner


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
