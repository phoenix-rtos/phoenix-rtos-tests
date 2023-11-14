#
# Phoenix-RTOS test runner
#
# The harness for the Busybox Test Suite
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau
#

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status


EXAMPLE_INPUT = """
---Testcase 'busybox as unknown name' starting---
echo -ne '' >input
echo -ne '' | ./unknown 2>&1
SKIPPED: busybox --help
UNTESTED: cp-parents
PASS: busybox as unknown name
---Testcase 'date-@-works' starting---
FAIL: date-@-works
+ TZ=EET-2EEST,M3.5.0/3,M10.5.0/4 busybox date -d @1288486799
+ test xSun Oct 31 03:59:59 EEST 2010 = xSun Oct 31 00:59:59  2010
---Testcase 'date-R-works' starting---
PASS: date-R-works
---Testcase 'bunzip2: doesnt exist' starting---
- t_actual differ: char 13, line 1
FAIL: bunzip2: doesnt exist
"""


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    subresult = None
    msg = []
    test_status = Status.OK

    START = r"---Testcase '(?P<name>.+?)' starting---\r+\n"
    RESULT = r"(?P<status>PASS|SKIPPED|UNTESTED|FAIL): (?P<name>.+?)\r+\n"
    FINAL = r"\*\*\*\*(The Busybox Test Suite completed|A single test of the Busybox Test Suite completed)\*\*\*\*\r+\n"
    MESSAGE = r"(?P<line>.*?)\r+\n"

    while True:
        idx = dut.expect([START, RESULT, FINAL, MESSAGE], timeout=90)
        parsed = dut.match.groupdict()

        if idx == 0:
            subresult = None
            continue

        if idx == 3:
            # "\n\t\t" used to create more readable multiline message when printed
            line_msg = "\n\t\t" + parsed["line"]
            # append extra message to the last test (if available)
            if subresult and subresult.status == Status.FAIL:
                subresult.msg += line_msg
            else:
                # message may also appear during the test
                msg.append(line_msg)

        if idx == 1:
            status = Status.from_str(parsed["status"])

            if status == Status.FAIL:
                test_status = Status.FAIL

            subresult = result.add_subresult(subname=parsed["name"], status=status, msg="".join(msg))
            msg.clear()

        elif idx == 2:
            break

    return TestResult(status=test_status)
