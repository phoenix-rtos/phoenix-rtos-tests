#
# Phoenix-RTOS test runner
#
# The harness for the Coremark Pro Test Suite
#
# Copyright 2025 Phoenix Systems
# Authors: Władysław Młynik
#

import os

import yaml
from pexpect.exceptions import EOF, TIMEOUT
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


def harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs):
    subresult = None
    msg = []

    script_dir = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(script_dir, "prepare_timeouts.yaml"), "r", encoding="utf-8") as f:
        prepare_timeouts = yaml.safe_load(f)

    with open(os.path.join(script_dir, "benchmark_timeouts.yaml"), "r", encoding="utf-8") as f:
        benchmark_timeouts = yaml.safe_load(f)

    target = ctx.target.name
    name = kwargs["name"].split("/")[-1]

    START = r"-  Info: Starting Run\.\.\.\r?\n"
    WORKLOAD = r"-- Workload:(?P<name>.*?)=(?P<size>.*?)\r?\n"
    MESSAGE = r"(?P<line>.*?)\r?\n"

    while True:
        try:
            idx = dut.expect([START, MESSAGE], timeout=prepare_timeouts[target][name])
        except (EOF, TIMEOUT) as e:
            line_msg = f"Error waiting for output: {type(e).__name__}"
            msg.append(line_msg)
            break
        parsed = dut.match.groupdict()

        if idx == 0:
            subresult = result.add_subresult(subname="prepare", status=Status.OK, msg="")
            break

        if idx == 1:
            # "\n\t\t" used to create more readable multiline message when printed
            line_msg = parsed["line"]
            msg.append(line_msg)

    if subresult is None:
        result.add_subresult(subname="prepare", status=Status.FAIL, msg="\n".join(msg))
        msg.clear()
        result.add_subresult(subname=name, status=Status.SKIP, msg="")
        return TestResult(status=Status.FAIL)

    subresult = None

    while True:
        try:
            idx = dut.expect([WORKLOAD, MESSAGE], timeout=benchmark_timeouts[target][name])
        except (EOF, TIMEOUT) as e:
            line_msg = f"Error waiting for output: {type(e).__name__}"
            msg.append(line_msg)
            break
        parsed = dut.match.groupdict()

        if idx == 0:
            subresult = result.add_subresult(subname=parsed["name"], status=Status.OK, msg="")
            break

        if idx == 1:
            # "\n\t\t" used to create more readable multiline message when printed
            line_msg = "\n\t\t" + parsed["line"]
            msg.append(line_msg)

    if subresult is None:
        result.add_subresult(subname=name, status=Status.FAIL, msg="\n".join(msg))
        return TestResult(status=Status.FAIL)
    return TestResult(status=Status.OK)
