#
# Phoenix-RTOS test runner
#
# The harness for the Busybox Test Suite
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau
#

from .common import TestResult


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
                if msg and test['status'] == TestResult.FAIL:
                    test['msg'] = msg
                    msg = ''

                test_results.append(TestResult(**test))

            if idx == 0:
                test = dict(zip(('status', 'name'), groups))
            elif idx == 1:
                break
            elif idx == 2:
                line = groups[0]
                msg += '\t\t' + line + '\n'

        return test_results
