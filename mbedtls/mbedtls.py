import re

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status

EXAMPLE_INPUT = """
MPS Reader: Single step, single round, pausing disabled ........... ----
Context init-free-init-free ....................................... PASS
net_poll beyond FD_SETSIZE ........................................ FAILED
  dup2( got_fd, wanted_fd ) >= 0
  at line 39, suites/test_suite_net.function
OID get Any Policy certificate policy ............................. PASS
"""


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    subresult = None
    test_status = Status.OK

    RESULT = r"(?P<name>.{67}\s)(?P<status>(PASS|----|FAILED))\r+\n"
    MSG_LINE = r"  (?P<line>[^\r\n]+?)\r+\n"
    FINAL = r"(?P<status>FAILED|PASSED)\s\((?P<nr>\d+)\s/\s(?P<total_nr>\d+)\stests\s\(\d+\sskipped\)\)"

    while True:
        idx = dut.expect([RESULT, FINAL, MSG_LINE], timeout=90)
        parsed = dut.match.groupdict()

        if idx == 2:
            # append extra message to the last test (if available)
            if subresult:
                subresult.msg += parsed["line"] + " "

        if idx == 0:
            # the subresult finished running - add it to result now to ensure correct run time
            if parsed["status"] == "----":
                status = Status.SKIP
            else:
                status = Status.from_str(parsed["status"])

            if status == Status.FAIL:
                test_status = Status.FAIL

            # if there are dots after test name - remove them
            subname = re.sub(r" \.+ ", "", parsed["name"])
            subresult = result.add_subresult(subname=subname, status=status)

        if idx == 1:
            break

    return TestResult(status=test_status)
