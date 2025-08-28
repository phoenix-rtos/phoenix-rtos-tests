import re

from trunner.dut import Dut
from trunner.ctx import TestContext
from trunner.types import Status, TestResult


def get_subtests_map(name: str, subtests_count: int):
    match = re.search(r"\{(.+?)\}", name)
    if not match:
        return None

    # Example of selected tests using curly braces: {1,3-6,8-12}
    selected_tests = match.group(1)

    nums = set()
    for part in selected_tests.split(","):
        if "-" in part:
            start, end = map(int, part.split("-"))
            nums.update(range(start, end + 1))
        else:
            nums.add(int(part))

    result = {i: (i in nums) for i in range(1, subtests_count + 1)}

    return result


def get_subtest_number(subtests_map):
    if subtests_map is None:
        for i in range(1, 100):
            yield i
    else:
        for key, value in subtests_map.items():
            if value is True:
                yield key


def harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> TestResult:
    tc_start = r"(.+?\|){2}TP Start\r+\n"
    tc_end = r"(.+?\|){2}IC End\r+\n"
    results = r"(.+?\|){2}(?P<status>PASS|FAIL|UNRESOLVED|UNSUPPORTED|NOTINUSE|UNTESTED|UNINITIATED|NORESULT|INVALID RESULT)\r+\n"  # noqa: E501
    final = r"(.+?\|){2}TC End.+?\r+\n"
    msg_line = r"(.+?\|){2}(?P<msg_line>.+?)\r+\n"

    stats = {"PASS": 0, "FAIL": 0, "SKIP": 0}
    get_msg, msg = False, ""
    substatus = None
    testcmd = kwargs.get("cmd")
    testname = testcmd.split("/")[-1]
    subtests_count = kwargs.get("testcase_count")
    subtests_map = get_subtests_map(testname, subtests_count)
    gen = get_subtest_number(subtests_map)
    old_n = n = 0

    dut.expect("Config End\r+\n")

    while True:
        idx = dut.expect([tc_start, results, tc_end, final, msg_line], timeout=2000)
        parsed = dut.match.groupdict()

        if idx == 0:
            substatus = Status.FAIL
            get_msg = True

        elif idx == 1:
            if parsed["status"] == "NOTINUSE":
                parsed["status"] = "SKIP"
            elif parsed["status"] not in ("FAIL", "PASS"):
                parsed["status"] = "FAIL"

            substatus = Status.from_str(parsed["status"])
            stats[parsed["status"]] += 1

        elif idx == 2:
            n = next(gen)

            if n > old_n:
                for i in range(old_n + 1, n):
                    subname = f"testcase {i}"
                    result.add_subresult(subname, status=Status.SKIP, msg="")

            subname = f"testcase {n}"
            result.add_subresult(subname, substatus, msg)
            get_msg, msg = False, ""
            old_n = n

        elif idx == 3:
            if n < subtests_count:
                for i in range(n + 1, subtests_count + 1):
                    subname = f"testcase {i}"
                    result.add_subresult(subname, status=Status.SKIP, msg="")

            break

        elif idx == 4 and get_msg:
            msg += ("" if not msg else "\n") + parsed["msg_line"]

    status = Status.FAIL if stats["FAIL"] != 0 or all(n == 0 for n in stats.values()) else Status.OK
    return TestResult(status=status)
