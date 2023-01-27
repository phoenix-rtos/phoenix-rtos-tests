import re
from typing import Sequence

from trunner.text import bold, red, yellow
from trunner.types import TestResult


def format_output(results: Sequence[dict]) -> str:
    output = []

    for res in results:
        if res["status"] == TestResult.OK:
            continue

        msg = "\t" + bold(res["name"]) + ": "
        if res["status"] == TestResult.FAIL:
            msg += red("FAIL") + "\n" + res["msg"]
        elif res["status"] == TestResult.SKIP:
            msg += yellow("SKIP") + "\n"

        output.append(msg)

    return "".join(output)


def harness(dut):
    results = []
    test = {}
    msg = ""
    test_status = TestResult.OK

    RESULT = r"(?P<name>.{67}\s)(?P<status>(PASS|----|FAILED))\r+\n"
    MSG_LINE = r"  (?P<line>[^\r\n]+?)\r+\n"
    FINAL = r"(?P<status>FAILED|PASSED)\s\((?P<nr>\d+)\s/\s(?P<total_nr>\d+)\stests\s\(\d+\sskipped\)\)"

    while True:
        idx = dut.expect([RESULT, FINAL, MSG_LINE], timeout=25)
        if idx == 1:
            break

        parsed = dut.match.groupdict()

        if test:
            if test["status"] == "----":
                test["status"] = TestResult.SKIP
            elif test["status"] == "FAILED":
                test["status"] = TestResult.FAIL
            elif test["status"] == "PASS":
                test["status"] = TestResult.OK

            # We ended processing test result and message
            if msg and test["status"] == TestResult.FAIL:
                test_status = TestResult.FAIL
                test["msg"] = msg
                msg = ""

            # if there are dots after test name - remove them
            dots = re.search(r" \.+ ", test["name"])
            if dots:
                test["name"] = test["name"][: dots.start()]

            results.append(test)

        if idx == 0:
            test = parsed
        elif idx == 2:
            msg += "\t\t" + parsed["line"] + "\n"

    return TestResult(msg=format_output(results), status=test_status)
