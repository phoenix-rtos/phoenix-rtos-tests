from typing import Any, Dict, Sequence

import trunner
from trunner.dut import Dut
from trunner.text import blue, bold, green, red, yellow
from trunner.types import TestResult


def _format_result_output(results: Sequence[Dict[str, Any]]) -> str:
    output = []
    for res in results:
        if res["status"] == "PASS" and trunner.ctx.verbosity == 0:
            continue

        out = "\t" + bold(f"TEST({res['group']}, {res['name']})") + " "
        if res["status"] == "FAIL":
            out += red("FAIL") + " at " + bold(f"{res['path']}:{res['line']}") + ": "
            out += res["msg"]
        elif res["status"] == "PASS":
            out += green("OK")
        elif res["status"] == "IGNORE":
            out += yellow("IGNORE")
        elif res["status"] == "INFO":
            out += blue("INFO")

        out += "\n"

        output.append(out)

    return "".join(output)


def unity_harness(dut: Dut):
    assert_re = r"ASSERTION (?P<path>.*?):(?P<line>\d+):(?P<status>FAIL|INFO|IGNORE): (?P<msg>.*?)\r"
    result_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>PASS|IGNORE)"
    # Fail need to have its own regex due to greedy matching
    result_final_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>FAIL) at (?P<path>.*?):(?P<line>\d+)\r"
    final_re = r"(?P<total>\d+) Tests (?P<fail>\d+) Failures (?P<ignore>\d+) Ignored"

    last_fail = {"path": None, "line": None}
    stats = {"FAIL": 0, "IGNORE": 0, "PASS": 0}
    results = []

    while True:
        idx = dut.expect([assert_re, result_re, result_final_re, final_re])
        parsed = dut.match.groupdict()

        if idx == 0:
            if parsed["status"] == "FAIL":
                last_fail = parsed
        elif idx in (1, 2):
            # TODO we do not consider INFO/IGNORE messages
            if parsed["status"] == "FAIL":
                if last_fail["path"] == parsed["path"] and last_fail["line"] == parsed["line"]:
                    parsed["msg"] = last_fail["msg"]

            stats[parsed["status"]] += 1
            results.append(parsed)
        elif idx == 3:
            for k, v in parsed.items():
                parsed[k] = int(v)

            assert (
                parsed["total"] == sum(stats.values())
                and parsed["fail"] == stats["FAIL"]
                and parsed["ignore"] == stats["IGNORE"]
            ), "There is a mismatch between the number of parsed tests and overall results!"

            # TODO parse last line
            break

    status = TestResult.FAIL if stats["FAIL"] != 0 else TestResult.OK
    return TestResult(msg=_format_result_output(results), status=status)
