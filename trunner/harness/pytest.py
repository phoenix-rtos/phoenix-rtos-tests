from typing import Optional

from trunner.ctx import TestContext
from trunner.dut import HostDut
from trunner.types import Status, TestResult


RESULT_TYPES = ['failed', 'passed', 'skipped', 'xfailed', 'xpassed', 'error', 'warnings']
CC = r"\x1b\[[0-9;]*m"


def pytest_harness(dut: HostDut, ctx: TestContext, result: TestResult, **kwargs) -> Optional[TestResult]:
    test_re = rf"::(?P<name>[^\x1b]+?)(?:{CC})+\s*(?P<status>PASSED|SKIPPED|FAILED|XFAIL|XPASS|ERROR)"
    final_re = r"\[100%\]"
    error_re = rf"(FAILED|ERROR)(?:{CC}).*::(?:{CC})(?P<name>.*)(?:{CC}) - (?P<msg>.*?\r\n)"
    summary_re = rf"=+ (?:{CC})+" + ''.join([rf"(?:(?P<{rt}>\d+) {rt}.*?)?" for rt in RESULT_TYPES]) + " in"

    test_path = ctx.project_path / kwargs.get("path")
    cmd = f"pytest -v --tb=no {test_path}"
    status = Status.OK

    dut.set_args(cmd, encoding="utf-8")
    dut.open()
    subresults = []
    tests = 0

    while True:
        idx = dut.expect([test_re, final_re, error_re, summary_re], timeout=200)
        parsed = dut.match.groupdict()

        if idx == 0:
            sub_status = Status.from_str(parsed["status"])
            if sub_status == Status.FAIL:
                status = sub_status

            subname = parsed['name']
            test = result.add_subresult(subname, sub_status)
            subresults.append(test)
            tests += 1

        elif idx == 1:
            pass

        elif idx == 2:
            for subresult in subresults:
                if parsed["name"] in subresult.subname:
                    subresult.msg = parsed["msg"]

        elif idx == 3:
            parsed_tests = sum(int(value) for value in parsed.values() if value is not None)
            assert (
                tests == parsed_tests
            ), "".join(("There is a mismatch between the number of parsed tests and overall results!\n",
                        f"Parsed results from the tests: {parsed_tests}",
                        f"Found test in summary line: {tests}"))
            break

    return TestResult(status=status, msg="")
