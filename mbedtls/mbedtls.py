import re

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status, TestSubResult


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    subresult = None
    msg = []
    test_status = Status.OK

    RESULT = r"(?P<name>.{67}\s)(?P<status>(PASS|----|FAILED))\r+\n"
    MSG_LINE = r"  (?P<line>[^\r\n]+?)\r+\n"
    FINAL = r"(?P<status>FAILED|PASSED)\s\((?P<nr>\d+)\s/\s(?P<total_nr>\d+)\stests\s\(\d+\sskipped\)\)"

    while True:
        idx = dut.expect([RESULT, FINAL, MSG_LINE], timeout=90)
        parsed = dut.match.groupdict()

        if idx == 2:
            msg.append(parsed["line"])
            continue

        if subresult:
            # We ended processing test result and message
            if msg and subresult.status == Status.FAIL:
                test_status = Status.FAIL
                subresult.msg = " ".join(msg)
                msg = []

            result.add_subresult_obj(subresult)
            subresult = None

        if idx == 0:
            if parsed["status"] == "----":
                status = Status.SKIP
            else:
                status = Status.from_str(parsed["status"])

            # if there are dots after test name - remove them
            subname = re.sub(r" \.+ ", "", parsed["name"])
            subresult = TestSubResult(subname=subname, status=status)
        if idx == 1:
            break

    return TestResult(status=test_status)
