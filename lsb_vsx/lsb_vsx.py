import re

from typing import Iterator

from trunner.dut import Dut
from trunner.ctx import TestContext
from trunner.types import Status, TestResult


def get_subtests_numbers(name: str) -> Iterator[int]:
    match = re.search(r"\{(.+?)\}", name)
    if not match:
        return iter(set(range(1, 100)))

    # Example of selected tests using curly braces: {1,3-6,8-12}
    selected_tests = match.group(1)

    nums = set()
    for part in selected_tests.split(","):
        if "-" in part:
            start, end = map(int, part.split("-"))
            nums.update(range(start, end + 1))
        else:
            nums.add(int(part))

    return iter(nums)


def harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> TestResult:
    start = r"^(.+?\|){2}TCM Start\r?\n"
    tc_start = r"^(.+?\|){2}TP Start\r?\n"
    status = r"^(.+?\|){2}(?P<status>PASS|FAIL|UNRESOLVED|UNSUPPORTED|NOTINUSE|UNTESTED|UNINITIATED|NORESULT|INVALID RESULT)\r?\n"  # noqa: E501
    tc_end = r"^(.+?\|){2}IC End\r?\n"
    final = r"^(.+?\|){2}TC End.+?\r?\n"
    msg_line = r"^(.+?\|){2}(?P<msg_line>.+?)\r?\n"
    vsx_error = r"(?P<vsx_error>error: .+?)\r?\n"

    stats = {"OK": 0, "FAIL": 0, "SKIP": 0}
    get_msg, msg = False, ""
    substatus = None
    testcmd = kwargs.get("cmd")
    testname = testcmd.split("/")[-1]
    subtests_count = kwargs.get("testcase_count")
    subtests_numbers = get_subtests_numbers(testname)
    curr_subtest_idx = next(subtests_numbers)
    prev_subtest_idx = 0

    while True:
        idx = dut.expect([start, tc_start, status, tc_end, final, msg_line, vsx_error], timeout=2000)
        parsed = dut.match.groupdict()

        # start
        if idx == 0:
            for i in range(prev_subtest_idx + 1, curr_subtest_idx):
                subname = f"testcase {i}"
                result.add_subresult(subname, status=Status.SKIP, msg="")

        # tc_start
        if idx == 1:
            substatus = Status.FAIL
            get_msg = True

        # status
        elif idx == 2:
            try:
                substatus = Status.from_str(parsed["status"])
            except ValueError:
                substatus = Status.FAIL

            stats[substatus.name] += 1

        # tc_end
        elif idx == 3:
            subname = f"testcase {curr_subtest_idx}"
            result.add_subresult(subname, substatus, msg)
            get_msg, msg = False, ""

            prev_subtest_idx = curr_subtest_idx
            curr_subtest_idx = next(subtests_numbers, subtests_count + 1)

            for i in range(prev_subtest_idx + 1, curr_subtest_idx):
                subname = f"testcase {i}"
                result.add_subresult(subname, status=Status.SKIP, msg="")

        # final
        elif idx == 4:
            break

        # msg_line
        elif idx == 5 and get_msg:
            msg += ("" if not msg else "\n") + parsed["msg_line"]

        # vsx_error
        elif idx == 6:
            msg += parsed["vsx_error"]
            return TestResult(status=Status.FAIL, msg=msg)

    status = Status.FAIL if stats["FAIL"] != 0 or all(n == 0 for n in stats.values()) else Status.OK
    return TestResult(status=status)
