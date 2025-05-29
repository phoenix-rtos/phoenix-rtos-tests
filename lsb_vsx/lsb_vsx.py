from typing import Optional

from trunner.dut import Dut
from trunner.ctx import TestContext
from trunner.types import Status, TestResult


def harness(dut: Dut, ctx: TestContext, result: TestResult) -> Optional[TestResult]:
    tc_start = r"(.+?\|){2}TP Start\r+\n"
    tc_end = r"(.+?\|){2}IC End\r+\n"
    results = r"(.+?\|){2}(?P<status>PASS|FAIL|UNRESOLVED|UNSUPPORTED|NOTINUSE|UNTESTED|UNINITIATED|NORESULT|INVALID RESULT)\r+\n"  # noqa: E501
    final = r"(.+?\|){2}TC End.+?\r+\n"
    msg_line = r"(.+?\|){2}(?P<msg_line>.+?)\r+\n"

    stats = {"FAIL": 0, "IGNORE": 0, "PASS": 0, "UNTESTED": 0}
    get_msg, msg = False, ""
    tc_num = 0

    dut.expect("Config End\r+\n")

    while True:
        idx = dut.expect([tc_start, results, tc_end, final, msg_line], timeout=2000)
        parsed = dut.match.groupdict()

        if idx == 0:
            get_msg = True

        elif idx == 1:
            if parsed["status"] in ("UNSUPPORTED", "NOTINUSE"):
                parsed["status"] = "IGNORE"
            elif parsed["status"] in ("UNRESOLVED", "UNINITIATED", "NORESULT"):
                # TODO: research pseudo-languages
                if msg and "pseudo language" not in msg:
                    parsed["status"] = "FAIL"
                else:
                    parsed["status"] = "IGNORE"
            elif parsed["status"] in ("INVALID RESULT"):
                parsed["status"] = "FAIL"

            status = Status.from_str(parsed["status"])
            stats[parsed["status"]] += 1

        elif idx == 2:
            tc_num += 1
            subname = f"testcase {tc_num}"
            result.add_subresult(subname, status, msg)
            get_msg, msg = False, ""

        elif idx == 3:
            break

        elif idx == 4 and get_msg:
            msg += ("" if not msg else "\n") + parsed["msg_line"]

    status = Status.FAIL if stats["FAIL"] != 0 or all(n == 0 for n in stats.values()) else Status.OK
    return TestResult(status=status)
