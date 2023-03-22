import re

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Result, Status


def harness(dut: Dut, ctx: TestContext):
    results = []
    result = None
    msg = []
    test_status = Status.OK

    RESULT = r"(?P<name>.{67}\s)(?P<status>(PASS|----|FAILED))\r+\n"
    MSG_LINE = r"  (?P<line>[^\r\n]+?)\r+\n"
    FINAL = r"(?P<status>FAILED|PASSED)\s\((?P<nr>\d+)\s/\s(?P<total_nr>\d+)\stests\s\(\d+\sskipped\)\)"

    while True:
        idx = dut.expect([RESULT, FINAL, MSG_LINE], timeout=25)
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

            # if there are dots after test name - remove them
            dots = re.search(r" \.+ ", result.name)
            if dots:
                result.name = result.name[: dots.start()]

            results.append(result)

        if idx == 0:
            if parsed["status"] == "----":
                status = Status.SKIP
            else:
                status = Status.from_str(parsed["status"])

            result = Result(name=parsed["name"], status=status)
        if idx == 1:
            break

    return TestResult(msg=Result.format_output(results, ctx), status=test_status)
