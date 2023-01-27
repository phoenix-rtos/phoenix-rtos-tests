#
# Phoenix-RTOS test runner
#
# The harness for the Busybox Test Suite
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau
#

from typing import Sequence

import trunner
from trunner.dut import Dut
from trunner.text import bold, green, red, yellow
from trunner.types import TestResult


def format_output(results: Sequence[dict]) -> str:
    output = []

    for res in results:
        if res["status"] != "FAIL" and trunner.ctx.verbosity == 0:
            continue

        msg = "\t" + bold(res["name"]) + ": "
        if res["status"] == "FAIL":
            msg += red("FAIL") + "\n" + res["msg"]
        elif res["status"] == "PASS":
            msg += green("OK") + "\n"
        elif res["status"] == "SKIPPED":
            msg += yellow("SKIP") + "\n"

        output.append(msg)

    return "".join(output)


def harness(dut: Dut):
    results = []
    test = {}
    msg = ""
    test_status = TestResult.OK

    RESULT = r"(?P<status>PASS|SKIPPED|FAIL): (?P<name>.+?)\r+\n"
    FINAL = r"\*\*\*\*(The Busybox Test Suite completed|A single test of the Busybox Test Suite completed)\*\*\*\*\r+\n"
    MESSAGE = r"(?P<line>.*?)\r+\n"

    while True:
        idx = dut.expect([RESULT, FINAL, MESSAGE], timeout=45)
        if idx == 1:
            break

        parsed = dut.match.groupdict()
        if test:
            # We ended processing test result and message
            if msg and test["status"] == "FAIL":
                test_status = TestResult.FAIL
                test["msg"] = msg
                msg = ""

            results.append(test)
            test = {}

        if idx == 0:
            test = parsed
        elif idx == 2:
            msg += "\t\t" + parsed["line"] + "\n"

    return TestResult(msg=format_output(results), status=test_status)
