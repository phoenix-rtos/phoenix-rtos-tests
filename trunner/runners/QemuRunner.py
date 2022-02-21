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

    def __init__(self, qemu, args, log):
        super().__init__(log)
        self.qemu = qemu
        self.args = args

    def flash(self):
        """No-op method for QemuRunner, forced inheritance from Runner class"""
        pass

    def run(self, test):
        if test.skipped():
            return

        proc = pexpect.spawn(self.qemu, args=self.args, encoding='ascii', timeout=test.timeout)
        if self.logpath:
            proc.logfile = open(self.logpath, "a")

        try:
            test.handle(proc)
        finally:
            proc.kill(signal.SIGTERM)
