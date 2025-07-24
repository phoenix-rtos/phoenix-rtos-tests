#
# Phoenix-RTOS test runner
#
# The harness for the Coremark Pro Test Suite
#
# Copyright 2025 Phoenix Systems
# Authors: Władysław Młynik
#

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult

EXAMPLE_INPUT = """
-  Info: Starting Run...
-- Workload:core=490760323
-- core:time(ns)=5221
-- core:contexts=1
-- core:iterations=1
-- core:time(secs)=   5.221
-- core:secs/workload=   5.221
-- core:workloads/sec=0.191534
-- Done:core=490760323
"""


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    subresult = None
    msg = []
    test_status = Status.OK

    prepare_timeouts = {
        "cjpeg-rose7-preset": 3,
        "loops-all-mid-10k-sp": 2,
        "radix2-big-64k": 10,
        "core": 2,
        "nnet_test": 2,
        "sha-test": 2,
        "linear_alg-mid-100x100-sp": 5,
        "parser-125k": 3,
        "zip-test": 100,
    }

    benchmark_timeouts = {
        "cjpeg-rose7-preset": 3,
        "loops-all-mid-10k-sp": 1800,
        "radix2-big-64k": 600,
        "core": 1500,
        "nnet_test": 7200,
        "sha-test": 3,
        "linear_alg-mid-100x100-sp": 7200,
        "parser-125k": 3,
        "zip-test": 3,
    }

    name = result.name.split("/")[-1]

    START = r"-  Info: Starting Run\.\.\."
    WORKLOAD = r"-- Workload:(?P<name>.*?)=(?P<size>.*?)\r*\n"
    FINAL = r"-- Done:(?P<name>.*?)=(?P<size>.*?)\r*\n"
    RESULT = r"-- (?P<name>.*?):(?P<metric>.*?)= *(?P<result>.*?)\r*\n"
    MESSAGE = r"(?P<line>.*?)\r*\n"

    while True:
        try:
            idx = dut.expect([START, MESSAGE], timeout=prepare_timeouts[name])
        except Exception:
            break
        parsed = dut.match.groupdict()

        if idx == 0:
            subresult = result.add_subresult(subname="prepare", status=Status.OK, msg="")
            break

        if idx == 1:
            # "\n\t\t" used to create more readable multiline message when printed
            line_msg = "\n\t\t" + parsed["line"]
            msg.append(line_msg)

    if subresult is None:
        subresult = result.add_subresult(subname="prepare", status=Status.FAIL, msg="".join(msg))
        subresult = result.add_subresult(subname=name, status=Status.SKIP, msg="")
        subresult = result.add_subresult(subname="time_within_limit", status=Status.SKIP, msg="")
        return TestResult(status=Status.FAIL)

    subresult = None

    while True:
        try:
            idx = dut.expect([WORKLOAD, MESSAGE], timeout=benchmark_timeouts[name])
        except Exception:
            break
        parsed = dut.match.groupdict()

        if idx == 0:
            subresult = result.add_subresult(subname=parsed["name"], status=test_status, msg="".join(msg))
            msg.clear()
            break

        if idx == 1:
            # "\n\t\t" used to create more readable multiline message when printed
            line_msg = "\n\t\t" + parsed["line"]
            msg.append(line_msg)

    if subresult is None:
        subresult = result.add_subresult(subname=name, status=Status.FAIL, msg="".join(msg))
        subresult = result.add_subresult(subname="time_within_limit", status=Status.SKIP, msg="")
        return TestResult(status=Status.FAIL)

    subresult = None

    while True:
        try:
            idx = dut.expect([FINAL, RESULT, MESSAGE], timeout=2)
        except Exception:
            break
        parsed = dut.match.groupdict()

        if idx == 1:
            if parsed["metric"] == "secs/workload" and float(parsed["result"]) > 30:
                test_status = Status.FAIL

        if idx == 0:
            subresult = result.add_subresult(subname="time_within_limit", status=test_status, msg="".join(msg))
            return TestResult(status=test_status)

        if idx == 2:
            # "\n\t\t" used to create more readable multiline message when printed
            line_msg = "\n\t\t" + parsed["line"]
            msg.append(line_msg)

    if subresult is None:
        subresult = result.add_subresult(subname="time_within_limit", status=Status.FAIL, msg="".join(msg))
        return TestResult(status=Status.FAIL)
