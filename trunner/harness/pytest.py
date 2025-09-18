import io
import re
from typing import Optional
from contextlib import redirect_stdout

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult

import pytest

RESULT_TYPES = ["failed", "passed", "skipped", "xfailed", "xpassed", "error", "warnings"]


def pytest_harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> Optional[TestResult]:
    test_re = r"::(?P<name>[^\x1b]+?) (?P<status>PASSED|SKIPPED|FAILED|XFAIL|XPASS|ERROR)"
    error_re = r"(FAILED|ERROR).*?::(?P<name>.*) - (?P<msg>.*)"
    summary_re = r"=+ " + "".join([rf"(?:(?P<{rt}>\d+) {rt}.*?)?" for rt in RESULT_TYPES]) + " in"

    test_path = ctx.project_path / kwargs.get("path")
    options = kwargs.get("options", "").split()
    status = Status.OK
    subresults = []
    tests = 0

    class TestContextPlugin:
        @pytest.fixture
        def dut(self):
            return dut

        @pytest.fixture
        def ctx(self):
            return ctx

        @pytest.fixture
        def kwargs(self):
            return kwargs

    test_args = [
        f"{test_path}",  # Path to test
        *options,
        "-v",  # Verbose output
        "--tb=no",
    ]

    output_buffer = io.StringIO()
    with redirect_stdout(output_buffer):
        pytest.main(test_args, plugins=[TestContextPlugin()])

    output = output_buffer.getvalue()

    if ctx.stream_output:
        print(output)

    for line in output.splitlines():
        match = re.search(test_re, line)
        error = re.search(error_re, line)
        final = re.search(summary_re, line)
        if match:
            parsed = match.groupdict()

            sub_status = Status.from_str(parsed["status"])
            if sub_status == Status.FAIL:
                status = sub_status

            subname = parsed["name"]
            test = result.add_subresult(subname, sub_status)
            subresults.append(test)
            tests += 1

        elif error:
            parsed = error.groupdict()
            for subresult in subresults:
                if parsed["name"] in subresult.subname:
                    subresult.msg = parsed["msg"]

        elif final:
            parsed = final.groupdict()
            parsed_tests = sum(int(value) for value in parsed.values() if value is not None)
            assert tests == parsed_tests, "".join(
                (
                    "There is a mismatch between the number of parsed tests and overall results!\n",
                    f"Parsed results from the tests: {parsed_tests}",
                    f"Found test in summary line: {tests}",
                )
            )

    return TestResult(status=status, msg="")
