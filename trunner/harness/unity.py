from typing import Optional

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult


def unity_harness(dut: Dut, ctx: TestContext, result: TestResult) -> Optional[TestResult]:
    assert_re = r"ASSERTION (?P<path>[\S]+):(?P<line>\d+):(?P<status>FAIL|INFO|IGNORE): (?P<msg>.*?)\r"
    result_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>PASS|IGNORE)"
    # Fail need to have its own regex due to greedy matching
    result_fail_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>FAIL) at (?P<path>.*?):(?P<line>\d+)\r"
    final_re = r"(?P<total>\d+) Tests (?P<fail>\d+) Failures (?P<ignore>\d+) Ignored \r+\n(?P<result>OK|FAIL)"

    last_assertion = {}
    stats = {"FAIL": 0, "IGNORE": 0, "PASS": 0}
    results = []

    while True:
        idx = dut.expect([assert_re, result_re, result_fail_re, final_re])
        parsed = dut.match.groupdict()

        if idx == 0:
            if parsed["status"] in ["FAIL", "IGNORE"]:
                last_assertion = parsed
        elif idx in (1, 2):
            if last_assertion:
                parsed["msg"] = last_assertion["msg"]
                last_assertion = {}

            status = Status.from_str(parsed["status"])
            subname = f"{parsed['group']}.{parsed['name']}"
            if "path" in parsed and "line" in parsed:
                parsed["msg"] = f"[{parsed['path']}:{parsed['line']}] " + parsed["msg"]
            result.add_subresult(subname, status, parsed.get("msg", ""))

            stats[parsed["status"]] += 1
            results.append(parsed)
        elif idx == 3:
            for k, v in parsed.items():
                if k != "result":
                    parsed[k] = int(v)

            assert (
                parsed["total"] == sum(stats.values())
                and parsed["fail"] == stats["FAIL"]
                and parsed["ignore"] == stats["IGNORE"]
            ), "".join(("There is a mismatch between the number of parsed tests and overall results!\n",
                        "Parsed results from the final Unity message (total, failed, ignored): ",
                        f"{parsed['total']}, {parsed['fail']}, {parsed['ignore']}\n",
                        "Found test summary lines (total, failed, ignored): ",
                        f"{sum(stats.values())}, {stats['FAIL']}, {stats['IGNORE']}"))

            break

    status = Status.FAIL if stats["FAIL"] != 0 else Status.OK
    return TestResult(status=status)
