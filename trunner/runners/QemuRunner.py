#
# Phoenix-RTOS test runner
#
# qemu emulator runner
#
# Copyright 2021 Phoenix SYstems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import pexpect
import signal

from .common import Runner
from trunner.config import PHRTOS_PROJECT_DIR


class QemuRunner(Runner):
    """This class provides interface to run test case using QEMU as a device."""

    QEMU_CMD = {
        'ia32-generic': f'{PHRTOS_PROJECT_DIR}/scripts/ia32-generic-test.sh',
        'armv7a9-zynq7000-qemu': f'{PHRTOS_PROJECT_DIR}/scripts/armv7a9-zynq7000.sh'
    }

    def __init__(self, target, log):
        super().__init__(log)
        self.cmd = self.QEMU_CMD[target]

    def flash(self):
        """No-op method for QemuRunner, forced inheritance from Runner class"""
        pass

    def run(self, test):
        if test.skipped():
            return

        proc = pexpect.spawn(self.cmd, encoding='utf-8', timeout=test.timeout)
        if self.logpath:
            proc.logfile = open(self.logpath, "a")

        try:
            test.handle(proc)
        finally:
            proc.kill(signal.SIGTERM)
