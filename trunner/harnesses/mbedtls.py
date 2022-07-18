#
# Phoenix-RTOS test runner
#
# The harness for the Mbedtls Test Suite
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau
#

import re

from .common import TestResult


class MbedtlsTestHarness:
    """Class providing harness for parsing output of Mbedtls tests"""

    RESULT = r"(?P<test_name>.{67}\s)(?P<status>(PASS|----|FAILED))\r+\n"
    MSG_LINE = r"  ([^\r\n]+?)\r+\n"
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
            ], timeout=25)
            groups = proc.match.groups()

            if idx != 2 and test:
                if test['status'] == '----':
                    test['status'] = TestResult.IGNORE
                if test['status'] == 'FAILED':
                    test['status'] = TestResult.FAIL
                # We ended processing test result and message
                if msg and test['status'] == TestResult.FAIL:
                    test['msg'] = msg
                    msg = ''

                # if there are dots after test name - remove them
                dots = re.search(r' \.+ ', test['name'])
                if dots:
                    test['name'] = test['name'][:dots.start()]

                test_results.append(TestResult(**test))

            if idx == 0:
                test = dict(zip(('name', 'status'), groups))
            elif idx == 1:
                break
            elif idx == 2:
                line = groups[0]
                msg += '\t\t' + line + '\n'

        return test_results
