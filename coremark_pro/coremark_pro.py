#
# Phoenix-RTOS test runner
#
# The harness for the Coremark Pro Test Suite
#
# Copyright 2025 Phoenix Systems
# Authors: Władysław Młynik
#

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

    prepare_timeouts = {
        "armv7a7-imx6ull-evk": {
            "cjpeg-rose7-preset": 2,
            "loops-all-mid-10k-sp": 2,
            "radix2-big-64k": 3,
            "core": 2,
            "nnet_test": 2,
            "sha-test": 2,
            "linear_alg-mid-100x100-sp": 2,
            "parser-125k": 3,
            "zip-test": 60,
        },
        "armv7a9-zynq7000-qemu": {
            "cjpeg-rose7-preset": 3,
            "loops-all-mid-10k-sp": 2,
            "radix2-big-64k": 10,
            "core": 2,
            "nnet_test": 2,
            "sha-test": 2,
            "linear_alg-mid-100x100-sp": 2,
            "parser-125k": 3,
            "zip-test": 30,
        },
        "armv7a9-zynq7000-zedboard": {
            "cjpeg-rose7-preset": 2,
            "loops-all-mid-10k-sp": 2,
            "radix2-big-64k": 3,
            "core": 2,
            "nnet_test": 2,
            "sha-test": 2,
            "linear_alg-mid-100x100-sp": 2,
            "parser-125k": 2,
            "zip-test": 60,
        },
        "armv7m4-stm32l4x6-nucleo": {
            "core": 2,
            "linear_alg-mid-100x100-sp": 10,
        },
        "armv7m7-imxrt106x-evk": {
            "cjpeg-rose7-preset": 2,
            "core": 2,
            "nnet_test": 2,
            "linear_alg-mid-100x100-sp": 2,
        },
        "armv7m7-imxrt117x-evk": {
            "cjpeg-rose7-preset": 2,
            "core": 2,
            "nnet_test": 2,
            "linear_alg-mid-100x100-sp": 2,
        },
        "ia32-generic-qemu": {
            "cjpeg-rose7-preset": 2,
            "loops-all-mid-10k-sp": 2,
            "radix2-big-64k": 3,
            "core": 3,
            "nnet_test": 2,
            "sha-test": 2,
            "linear_alg-mid-100x100-sp": 2,
            "parser-125k": 2,
            "zip-test": 30,
        },
        "riscv64-generic-qemu": {
            "cjpeg-rose7-preset": 2,
            "loops-all-mid-10k-sp": 2,
            "radix2-big-64k": 3,
            "core": 2,
            "nnet_test": 2,
            "sha-test": 2,
            "linear_alg-mid-100x100-sp": 2,
            "parser-125k": 2,
            "zip-test": 30,
        },
    }

    benchmark_timeouts = {
        "armv7a7-imx6ull-evk": {
            "cjpeg-rose7-preset": 3,
            "loops-all-mid-10k-sp": 200,
            "radix2-big-64k": 100,
            "core": 10,
            "nnet_test": 40,
            "sha-test": 3,
            "linear_alg-mid-100x100-sp": 9,
            "parser-125k": 3,
            "zip-test": 3,
        },
        "armv7a9-zynq7000-qemu": {
            "cjpeg-rose7-preset": 3,
            "loops-all-mid-10k-sp": 1800,
            "radix2-big-64k": 600,
            "core": 1500,
            "nnet_test": 40,
            "sha-test": 3,
            "linear_alg-mid-100x100-sp": 7200,
            "parser-125k": 3,
            "zip-test": 3,
        },
        "armv7a9-zynq7000-zedboard": {
            "cjpeg-rose7-preset": 3,
            "loops-all-mid-10k-sp": 180,
            "radix2-big-64k": 50,
            "core": 10,
            "nnet_test": 40,
            "sha-test": 3,
            "linear_alg-mid-100x100-sp": 10,
            "parser-125k": 3,
            "zip-test": 3,
        },
        "armv7m4-stm32l4x6-nucleo": {
            "core": 800,
            "linear_alg-mid-100x100-sp": 5000,
        },
        "armv7m7-imxrt106x-evk": {
            "cjpeg-rose7-preset": 3,
            "core": 10,
            "nnet_test": 75,
            "linear_alg-mid-100x100-sp": 10,
        },
        "armv7m7-imxrt117x-evk": {
            "cjpeg-rose7-preset": 3,
            "core": 8,
            "nnet_test": 50,
            "linear_alg-mid-100x100-sp": 8,
        },
        "ia32-generic-qemu": {
            "cjpeg-rose7-preset": 5,
            "loops-all-mid-10k-sp": 1200,
            "radix2-big-64k": 300,
            "core": 10,
            "nnet_test": 300,
            "sha-test": 5,
            "linear_alg-mid-100x100-sp": 80,
            "parser-125k": 5,
            "zip-test": 5,
        },
        "riscv64-generic-qemu": {
            "cjpeg-rose7-preset": 3,
            "loops-all-mid-10k-sp": 140,
            "radix2-big-64k": 40,
            "core": 7,
            "nnet_test": 40,
            "sha-test": 3,
            "linear_alg-mid-100x100-sp": 7,
            "parser-125k": 3,
            "zip-test": 3,
        },
    }
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
            msg.append(parsed["line"])

    if subresult is None:
        result.add_subresult(subname="prepare", status=Status.FAIL, msg="\n".join(msg))
        result.add_subresult(subname=name, status=Status.SKIP, msg="Benchmark skipped because preparation failed")
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
            msg.append(parsed["line"])

    if subresult is None:
        result.add_subresult(subname=name, status=Status.FAIL, msg="\n".join(msg))
        return TestResult(status=Status.FAIL)
    return TestResult(status=Status.OK)
