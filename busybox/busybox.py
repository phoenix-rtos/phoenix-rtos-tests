#
# Phoenix-RTOS test runner
#
# The harness for the Busybox Test Suite
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau
#

from trunner.dut import Dut
from trunner.types import TestResult, Result, Status


def harness(dut: Dut):
    results = []
    result = None
    msg = []
    test_status = Status.OK

    RESULT = r"(?P<status>PASS|SKIPPED|FAIL): (?P<name>.+?)\r+\n"
    FINAL = r"\*\*\*\*(The Busybox Test Suite completed|A single test of the Busybox Test Suite completed)\*\*\*\*\r+\n"
    MESSAGE = r"(?P<line>.*?)\r+\n"

    while True:
        idx = dut.expect([RESULT, FINAL, MESSAGE], timeout=45)
        parsed = dut.match.groupdict()

        if idx == 2:
            msg.append("\t\t" + parsed["line"])
            continue

        if result:
            # We ended processing test result and message
            if msg and result.status == Status.FAIL:
                test_status = Status.FAIL
                result.msg = "\n".join(msg)
                msg = []

            results.append(result)
            result = None

        if idx == 0:
            result = Result(name=parsed["name"], status=Status.from_str(parsed["status"]))
        elif idx == 1:
            break

    return TestResult(msg=Result.format_output(results), status=test_status)
