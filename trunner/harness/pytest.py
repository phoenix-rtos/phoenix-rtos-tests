import os
import re
import select
import shlex
import sys
from collections.abc import Generator
from typing import Any

import pluggy
import pytest
from pytest import Function, TestReport
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.text import bold, green, red, yellow
from trunner.types import Status, TestResult


def extract_reason_from_report(report: TestReport) -> str:
    reason = ""
    if isinstance(report.longrepr, tuple):
        reason = report.longrepr[2].replace("Skipped: ", "")
    elif hasattr(report, "wasxfail"):
        reason = report.wasxfail
        reason = f"XFAIL{': ' if len(reason) > 0 else ''}{reason}"
    return reason.strip()


def get_status(report: TestReport) -> str:
    was_xfail = hasattr(report, "wasxfail")
    if report.failed and report.when != "call":
        return red("ERROR")

    if report.passed and not was_xfail:
        return green("PASSED")
    if report.passed and was_xfail:
        return yellow("XPASS")
    if report.failed:
        return red("FAILED")
    if report.skipped:
        return yellow("XFAIL" if was_xfail else "SKIPPED")

    return bold("UNKNOWN")


def get_str_attr(obj: object, attribute: str):
    raw_attr = getattr(obj, attribute, "")
    raw_attr = "" if raw_attr is None else raw_attr

    attribute_str = raw_attr.decode("utf-8", "ignore") if isinstance(raw_attr, bytes) else str(raw_attr)
    return attribute_str.replace("\r", "")


class PytestLogInterceptorPlugin:
    """Plugin that intercepts and modifies the log output

    It modifies only the raw PyTest output, not the live
    output coming from the DUT, etc.
    """
    def __init__(self, ctx: TestContext) -> None:
        self._ctx = ctx
        self._screen_writer = None
        self._first_test_ran = False

    @pytest.hookimpl(trylast=True)
    def pytest_configure(self, config: Any) -> None:
        terminal = config.pluginmanager.get_plugin("terminalreporter")
        if not terminal:
            return

        if self._ctx.stream_output:
            self._screen_writer = type(terminal._tw)(sys.stdout)

        # Redirects PyTest output to /dev/null
        terminal._tw._file = open(os.devnull, "w")

        config.option.disable_warnings = True
        config.option.no_summary = True
        terminal.summary_stats = lambda: None

    def pytest_sessionstart(self, session: Any):
        if self._screen_writer:
            self._screen_writer.write("\n")

    @pytest.hookimpl(tryfirst=True)
    def pytest_runtest_logstart(self, nodeid: str) -> None:
        """Manually print start header to screen.

        Retains pexpect post-initalization buffer for the first
        test-case, based on an assumption that pexpect object is
        spawned only once within a test-suite.
        """
        if self._screen_writer is None:
            return
        if self._first_test_ran:
            self._ensure_newline()
        else:
            self._first_test_ran = True
        self._screen_writer.write(f"\t{bold(nodeid)}...")
        self._screen_writer.write("\n")

    @pytest.hookimpl(tryfirst=True)
    def pytest_runtest_logreport(self, report: TestReport) -> None:
        """Manually print status to screen."""
        should_print = (report.when == "call") or (not report.passed)

        if not should_print or self._screen_writer is None:
            return

        status = get_status(report)
        reason = extract_reason_from_report(report)

        self._ensure_newline()

        self._screen_writer.write(f"\t{bold(report.nodeid)} ")
        self._screen_writer.write(status)

        if reason:
            self._screen_writer.write(f" ({reason})")

        self._screen_writer.write("\n")

    def _ensure_newline(self) -> None:
        """Check if there was a newline before and write it if not

        Sometimes there is no newline at the end of the buffer and
        we need to add it to preserve the correct output formatting
        """
        dut = getattr(self._ctx.target, "dut", None)
        pexpect_p = getattr(dut, "pexpect_proc", None)
        if dut is None or pexpect_p is None:
            return
        readers, _, _ = select.select([pexpect_p.fileno()], [], [], 0)
        if readers:
            dut.clear_buffer()

        after = getattr(pexpect_p, "after", None)
        if getattr(self, "_last_handled_after", object()) is after or after is None:
            return
        self._last_handled_after = after

        if self._screen_writer and isinstance(after, str) and not after.endswith("\n"):
            self._screen_writer.write("\n")


class PytestBridgePlugin:
    """Plugin that bridges the TestRunner and PyTest frameworks.

    Each test case is run as TestRunner's subtest, accurately
    capturing each subresult's runtime and status.
    """

    _unknown_fixture_str = "[UNKNOWN FIXTURE]"
    _common_fast_fail_str = "Requires a previously failing fixture:"

    def __init__(self, dut: Dut, ctx: TestContext, result: TestResult, kwargs: dict) -> None:
        self._dut = dut
        self._ctx = ctx
        self._result = result
        self._kwargs = kwargs
        self._pending_result = (Status.OK, "")
        self._broken_fixtures: set[str] = set()

    @pytest.fixture(scope="session")
    def dut(self) -> Dut:
        return self._dut

    @pytest.fixture(scope="session")
    def ctx(self) -> TestContext:
        return self._ctx

    @pytest.fixture(scope="session")
    def kwargs(self) -> dict:
        return self._kwargs

    @pytest.hookimpl(tryfirst=True)
    def pytest_runtest_setup(self, item: Function) -> None:
        """Fail current test case immediately, if a required fixture failed
        during previous test case's setup stage
        """
        for fixture in item.fixturenames:
            if fixture in self._broken_fixtures:
                pytest.fail(f"{self._common_fast_fail_str} {fixture}", pytrace=False)

    @pytest.hookimpl(hookwrapper=True)
    def pytest_runtest_makereport(self, item: Function, call: pytest.CallInfo) -> Generator[None, Any, None]:
        """Post-process PyTest's report making

        Inject output caught before exception for a more detailed report.

        Currently, TestResult failing methods fail the entire test with the
        most recent exception, with no regard to individual subresults.

        TODO: Modify TestResult failing methods to support single subresult
        exceptions and use them here to avoid reimplementation.
        """
        outcome: pluggy.Result = yield
        report: TestReport = outcome.get_result()

        if call.when != "call" or not report.failed or not call.excinfo:
            return

        exc_instance = call.excinfo.value
        report_add_info = None

        before = get_str_attr(self._dut, "before")

        if isinstance(exc_instance, UnicodeDecodeError) and before:
            report_add_info = bold("OUTPUT CAUGHT BEFORE EXCEPTION:")
            report_add_info += before

        if report_add_info:
            report.longrepr = str(report.longrepr) + "\n\n" + report_add_info

    @pytest.hookimpl()
    def pytest_runtest_logreport(self, report: TestReport) -> None:
        """PyTest hook called after a test phase completion.

        Adds a TestRunner subresult for each PyTest case report

        :param report: PyTest's TestReport object
        """
        error_msg = getattr(report, "longreprtext", str(report.longrepr)) if report.longrepr else ""

        if report.when == "setup":
            self._record_setup_phase(report, error_msg)

        if report.when == "call":
            self._record_call_phase(report, error_msg)

        if report.when == "teardown":
            self._finalize_and_report(report, error_msg)

    def _record_setup_phase(self, report: TestReport, error_msg: str) -> None:
        if report.failed:
            fixture_name = self._extract_broken_fixture(error_msg)
            self._pending_result = (Status.FAIL, f"{error_msg}")
            if fixture_name != self._unknown_fixture_str:
                # It is assumed that fixtures failing at startup should
                # not be re-run, thus marking them as `broken`
                self._broken_fixtures.add(fixture_name)
        elif report.skipped:
            skip_msg = extract_reason_from_report(report)
            self._pending_result = (Status.SKIP, skip_msg)

    def _record_call_phase(self, report: TestReport, error_msg: str) -> None:
        status = Status.OK
        msg = ""

        if report.failed:
            status = Status.FAIL
            msg = error_msg
        elif report.skipped:
            # XFAIL is treated as SKIP to not confuse it with success but
            # it is not a FAIL either. XPASS will still be treated as OK
            status = Status.SKIP
            msg = extract_reason_from_report(report)

        self._pending_result = (status, msg)

    def _finalize_and_report(self, report: TestReport, error_msg: str) -> None:
        stored = self._pending_result
        final_status = stored[0]
        final_msg = stored[1]
        teardown_msg = error_msg

        if report.failed:
            final_status = Status.FAIL

            if final_msg:
                final_msg = f"{final_msg}\n{teardown_msg}"
            else:
                final_msg = f"{teardown_msg}"

        self._result.add_subresult(subname=report.nodeid.split("::")[-1], status=final_status, msg=final_msg)

    @classmethod
    def _extract_broken_fixture(cls, text: str) -> str:
        fast_fail_match = re.search(rf"{re.escape(cls._common_fast_fail_str)} (\w+)", text)
        if fast_fail_match:
            return fast_fail_match.group(1)
        decorator_match = re.search(r"@pytest\.fixture.*?def\s+(\w+)", text, re.DOTALL)
        if decorator_match:
            return decorator_match.group(1)
        return cls._unknown_fixture_str


def pytest_harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> TestResult:
    if "script" not in kwargs:
        result.fail(msg="missing `script` from test configuration!")
        return result

    test_path = ctx.project_path / kwargs["script"]
    options = shlex.split(kwargs.get("options", ""))

    bridge_plugin = PytestBridgePlugin(dut, ctx, result, kwargs)
    log_plugin = PytestLogInterceptorPlugin(ctx)

    cmd_args = [str(test_path), *options, "-s"]

    try:
        exit_code = pytest.main(cmd_args, plugins=[bridge_plugin, log_plugin])
    except Exception:
        result.fail_unknown_exception()
        return result

    accepted_codes = (pytest.ExitCode.OK, pytest.ExitCode.TESTS_FAILED, pytest.ExitCode.NO_TESTS_COLLECTED)

    if exit_code not in accepted_codes:
        result.status = Status.FAIL
        result.msg = f"Pytest execution failed (Exit Code {exit_code})."

    return result
