import pexpect
import re

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    test_status = Status.OK
    msg = ""

    RESULT = r"(?P<name>.{67}\s)(?P<status>(PASS|----|FAILED))\r+\n"
    MSG_LINE = r"  (?P<line>[^\r\n]+?)\r+\n"
    FINAL = r"(?P<status>FAILED|PASSED)\s\((?P<nr>\d+)\s/\s(?P<total_nr>\d+)\stests\s\(\d+\sskipped\)\)"

    while True:
        idx = dut.expect([RESULT, FINAL], timeout=90)
        parsed = dut.match.groupdict()

        if idx == 0:
            if parsed["status"] == "----":
                status = Status.SKIP
            else:
                status = Status.from_str(parsed["status"])

            if status == Status.FAIL:
                test_status = status
                # Capture fail message if any exists
                while True:
                    try:
                        dut.expect([MSG_LINE], timeout=1)
                        msg += dut.match.groupdict()["line"] + "\n\r\t\t"

                    except pexpect.TIMEOUT:
                        msg = msg[:-4]
                        break

            # if there are dots after test name - remove them
            subname = re.sub(r" \.+ ", "", parsed["name"])
            result.add_subresult(subname=subname, status=status, msg=msg)
            msg = ""

        if idx == 1:
            break

    return TestResult(status=test_status)
