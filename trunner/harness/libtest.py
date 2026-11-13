import json
import re
from typing import Dict, List, Optional

import pexpect

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult


# libtest's JSON output, an unstable option (the Phoenix toolchain is not a stable one)
LIBTEST_ARGS = ["-Z", "unstable-options", "--format", "json"]

# { "type": "test", "event": "started", "name": "tests::a" }
EVENT_RE = r'\{ "type": "(?:suite|test|bench)",[^\r\n]*\}\r*\n'

STATUS = {"ok": Status.OK, "failed": Status.FAIL, "timeout": Status.FAIL, "ignored": Status.SKIP}


def libtest_harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> Optional[TestResult]:
    """Harness for `cargo test` binaries run with LIBTEST_ARGS; #[test] functions are subresults.

    kwargs: timeout - seconds to wait for the next event (default 60)
    """

    timeout = int(kwargs.get("timeout", 60))
    prompt = re.escape(ctx.target.shell_prompt.strip())

    statuses: Dict[str, Status] = {}
    test_count = None
    # more than one when tests run in parallel
    running: List[str] = []

    while True:
        try:
            idx = dut.expect([EVENT_RE, prompt], timeout=timeout)
        except pexpect.TIMEOUT:
            what = f"result of {', '.join(running)}" if running else "libtest event"
            return TestResult(msg=f"no {what} within {timeout}s, output:\n{dut.before}", status=Status.FAIL)

        if idx == 1:
            # the binary's output since the last event: psh's error, a kernel exception, a panic
            output = dut.before.replace("\r", "").strip()
            missing = re.search(r"^psh: (\S+) not found$", output, re.MULTILINE)
            if missing and test_count is None:
                msg = f"the test binary {missing.group(1)} is not on the target"
            else:
                if test_count is None:
                    msg = "the test binary exited before libtest started"
                else:
                    msg = "the test binary exited without the libtest summary"
                if running:
                    msg += f", running: {', '.join(running)}"
                msg += f", output:\n{output}" if output else ", without output"
            return TestResult(msg=msg, status=Status.FAIL)

        line = dut.match.group(0).strip()
        try:
            event = json.loads(line)
        except json.JSONDecodeError as e:
            raise AssertionError(f"malformed libtest event (mixed with other output?): {line}") from e

        kind, name = event["event"], event.get("name")

        if event["type"] == "suite":
            if kind == "started":
                test_count = event["test_count"]
                continue
            break

        if kind == "started":
            running.append(name)
            continue

        if name in running:
            running.remove(name)
        status = STATUS[kind]
        statuses[name] = status
        # stdout: output and panic of a failed test, message: e.g. an ignore reason
        msg = "\n".join(event[k].strip() for k in ("message", "stdout") if event.get(k))
        result.add_subresult(name, status, msg)

    counts = {s: sum(1 for v in statuses.values() if v is s) for s in Status}
    assert (
        counts[Status.OK] == event["passed"]
        and counts[Status.FAIL] == event["failed"]
        and counts[Status.SKIP] == event["ignored"]
        and (test_count is None or test_count == len(statuses))
    ), (
        "There is a mismatch between the parsed tests and the libtest summary!\n"
        f"Summary (passed, failed, ignored): {event['passed']}, {event['failed']}, {event['ignored']}\n"
        f"Parsed: {counts[Status.OK]}, {counts[Status.FAIL]}, {counts[Status.SKIP]}"
    )

    failed = event["event"] != "ok" or counts[Status.FAIL] != 0
    return TestResult(status=Status.FAIL if failed else Status.OK)
