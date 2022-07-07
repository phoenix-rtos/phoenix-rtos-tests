#
# Phoenix-RTOS test runner
#
# Long Test Module - Test Runner support for the phoenix-rtos-ports test suites
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau
#

from .tools.color import Color


LONG_TESTS = ['busybox', 'mbedtls']


class LongTestHarnessFactory:
    @staticmethod
    def create(ltest_type):
        if (ltest_type == 'busybox'):
            return BusyboxTestHarness.harness
        if (ltest_type == 'mbedtls'):
            return MbedtlsTestHarness.harness
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


class MbedtlsTestHarness:
    """Class providing harness for parsing output of Mbedtls tests"""

    # common result line: 'Hash: RIPEMD160 ................................................... PASS'
    RESULT = r"(?P<test_name>.{67})\s(?P<status>(PASS|----|FAILED))\r+\n"
    # after failed test case there is some additional output
    MSG_LINE = r"  ([^\r\n]+?)\r+\n"
    # common final line: 'FAILED (16 / 21 tests (6 skipped))'
    FINAL = r"(?P<status>FAILED|PASSED)\s\((?P<nr>\d+)\s/\s(?P<total_nr>\d+)\stests\s\(\d+\sskipped\)\)"

    @staticmethod
    def harness(proc):
        test_results = []
        test = None
        msg = ""

        while True:
            idx = proc.expect([
                MbedtlsTestHarness.RESULT,
                MbedtlsTestHarness.FINAL,
                MbedtlsTestHarness.MSG_LINE
            ], timeout=15)
            groups = proc.match.groups()
            if idx != 2 and test:
                if test['status'] == '----':
                    test['status'] = 'SKIPPED'
                if test['status'] == 'FAILED':
                    test['status'] = 'FAIL'
                # We ended processing test result and message
                if msg and test['status'] == 'FAIL':
                    test['msg'] = msg
                    msg = ''

                test['name'] = test['name'].replace('.', '')
                test_results.append(LongTestResult(**test))
            else:
                line = groups[0]
                msg += '\t\t' + line + '\n'

            if idx == 0:
                test = dict(zip(('name', 'status'), groups))
            elif idx == 1:
                break

        return test_results


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
