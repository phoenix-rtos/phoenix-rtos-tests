#
# Phoenix-RTOS test runner
#
# The harness for the Busybox Test Suite
#
# Copyright 2022, 2023 Phoenix Systems
# Authors: Damian Loewnau, Mateusz Bloch
#
import pexpect

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    msg = ""
    test_status = Status.OK

    RESULT = r"(?P<status>PASS|SKIPPED|FAIL): (?P<name>.+?)\r+\n"
    FINAL = r"\*\*\*\*(The Busybox Test Suite completed|A single test of the Busybox Test Suite completed)\*\*\*\*\r+\n"
    MESSAGE = r"^[+-](?P<line>.*?)\r+\n"

    while True:
        idx = dut.expect([RESULT, FINAL, MESSAGE], timeout=90)
        parsed = dut.match.groupdict()

        if idx == 2:
            # BusyBox tests, a message can appear either before or after the RESULT.
            msg += "-" + parsed["line"] + "\n\r\t\t"

        if idx == 0:
            status = Status.from_str(parsed["status"])

            if status == Status.FAIL:
                test_status = status
                # Capture fail message after RESULT if any exists
                while True:
                    try:
                        dut.expect([MESSAGE], timeout=1)
                        msg += "+" + dut.match.groupdict()["line"] + "\n\r\t\t"

                    except pexpect.TIMEOUT:
                        msg = msg[:-4]
                        break

            result.add_subresult(subname=parsed["name"], status=status, msg=msg)
            msg = ""

        if idx == 1:
            break

    return TestResult(status=test_status)
