#
# Phoenix-RTOS test runner
#
# Harnesses for test frameworks supported in the Test Runner - common
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau, Jakub Sarzyński
#

from trunner.tools.color import Color


class TestResult:
    """Class representing results of one of the 3rd party frameworks. """

    PASS = 'PASS'
    FAIL = 'FAIL'
    IGNORE = 'SKIPPED'

    def __init__(self, name, status, msg=''):
        self.name = name
        self.status = status
        self.msg = msg

    def __str__(self):
        color = Color.OK

        if self.status == TestResult.FAIL:
            color = Color.FAIL
        elif self.status == TestResult.IGNORE:
            color = Color.SKIP

        status = Color.colorify(self.status, color)

        res = f"{status}: {self.name}"
        if self.status == TestResult.FAIL:
            res += '\n\t\tOutput of the failed test case:\n\t\t---\n'
            res += f'{self.msg}'
            res += '\t\t---'
        return res
