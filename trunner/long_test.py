#
# Phoenix-RTOS test runner
#
# Long Test Module - Test Runner support for the phoenix-rtos-ports test suites
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau
#

from .tools.color import Color


LONG_TESTS = ['busybox', ]


class LongTestHarnessFactory:
    @staticmethod
    def create(ltest_type):
        if (ltest_type == 'busybox'):
            return BusyboxTestHarness.harness
        else:
            raise ValueError(f"Unknown long test type: {ltest_type}")


class LongTestResult:
    """Class representing results of one of the long test suites."""

    PASS = 'PASS'
    FAIL = 'FAIL'
    IGNORE = 'SKIPPED'

    def __init__(self, name, status, msg=''):
        self.name = name
        self.status = status
        self.msg = msg

    def __str__(self):
        if self.status == LongTestResult.PASS:
            color = Color.OK
        elif self.status == LongTestResult.FAIL:
            color = Color.FAIL
        elif self.status == LongTestResult.IGNORE:
            color = Color.SKIP

        status = Color.colorify(self.status, color)

        res = f"{status}: {self.name}"
        if self.status == LongTestResult.FAIL:
            res += '\n\t\tOutput of the failed test case:\n\t\t---\n'
            res += f'{self.msg}'
            res += '\t\t---'
        return res


class BusyboxTestHarness:
    """Class providing harness for parsing output of the Busybox test suite"""

    RESULT = r"(PASS|SKIPPED|FAIL): (.+?)\r+\n"
    FINAL = r"\*\*\*\*(The Busybox Test Suite completed|A single test of the Busybox Test Suite completed)\*\*\*\*\r+\n"
    MESSAGE = r"(.*?)\r+\n"

    @staticmethod
    def harness(proc):
        test_results = []
        test = None
        msg = ""

        while True:
            idx = proc.expect([
                BusyboxTestHarness.RESULT,
                BusyboxTestHarness.FINAL,
                BusyboxTestHarness.MESSAGE
            ], timeout=45)
            groups = proc.match.groups()
            if idx != 2 and test:
                # We ended processing test result and message
                if msg and test['status'] == 'FAIL':
                    test['msg'] = msg
                    msg = ''

                test_results.append(LongTestResult(**test))

            if idx == 0:
                test = dict(zip(('status', 'name'), groups))
            elif idx == 1:
                break
            elif idx == 2:
                line = groups[0]
                msg += '\t\t' + line + '\n'

        return test_results
