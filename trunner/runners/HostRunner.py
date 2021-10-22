#
# Phoenix-RTOS test runner
#
# host-pc runner
#
# Copyright 2021 Phoenix SYstems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import pexpect
import signal

from .common import Runner
from .common import rootfs


class HostRunner(Runner):
    """This class provides interface to run test case using host as a device."""

    def run(self, test):
        if test.skipped():
            return

        test_path = rootfs(test.target) / 'bin' / test.exec_cmd[0]

        try:
            proc = pexpect.spawn(
                str(test_path),
                args=test.exec_cmd[1:],
                encoding='utf-8',
                timeout=test.timeout
            )
        except pexpect.exceptions.ExceptionPexpect:
            test.handle_exception()
            return

        try:
            test.handle(proc, psh=False)
        finally:
            proc.kill(signal.SIGTERM)
