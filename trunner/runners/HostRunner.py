#
# Phoenix-RTOS test runner
#
# host-pc runner
#
# Copyright 2021 Phoenix SYstems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import pexpect

from .common import Runner
from .common import rootfs


class HostRunner(Runner):
    """This class provides interface to run test case using host as a device."""

    def flash(self):
        """No-op method for HostRunner, forced inheritance from Runner class"""
        pass

    def run(self, test):
        if test.skipped():
            return

        test_path = rootfs(test.target) / 'bin' / test.exec_cmd[0]

        # host-pc tests must not use psh prompt
        test.psh = False

        try:
            with pexpect.spawn(
                str(test_path),
                args=test.exec_cmd[1:],
                encoding="utf-8",
                timeout=test.timeout,
            ) as proc:
                if self.logpath:
                    proc.logfile = open(self.logpath, "w")
                test.handle(proc)
        except pexpect.exceptions.ExceptionPexpect:
            test.handle_exception()
