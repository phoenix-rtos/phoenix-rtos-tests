from typing import Optional

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult


def harness(dut: Dut, ctx: TestContext, result: TestResult) -> Optional[TestResult]:
    tc_start_re = r"(.+?\|){2}TP Start\r+\n"
    tc_end_re = r"(.+?\|){2}IC End\r+\n"
    result_re = r"(.+?\|){2}(?P<status>PASS|FAIL|UNRESOLVED|UNSUPPORTED|NOTINUSE|UNTESTED|UNINITIATED|NORESULT|INVALID RESULT)\r+\n"  # noqa: E501
    final_re = r"(.+?\|){2}TC End.+?\r+\n"
    msg_re = r"(.+?\|){2}(?P<msg_line>.+?)\r+\n"

    stats = {"FAIL": 0, "IGNORE": 0, "PASS": 0, "UNTESTED": 0}
    msg = ""
    results = []
    tc_num = 0
    get_msg = False

    dut.expect("Config End\r+\n")

    while True:
        idx = dut.expect([tc_start_re, result_re, tc_end_re, final_re, msg_re], timeout=2000)
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
            results.append(parsed)
        elif idx == 2:
            tc_num += 1
            subname = f"testcase {tc_num}"
            result.add_subresult(subname, status, msg)
            get_msg, msg = False, ""
        elif idx == 3:
            break
        elif idx == 4 and get_msg is True:
            msg += parsed["msg_line"] + "\n"

    status = Status.FAIL if stats["FAIL"] != 0 else Status.OK
    return TestResult(status=status)
