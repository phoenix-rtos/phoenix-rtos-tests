import os
import re
import sys
from collections.abc import Generator
from typing import Any

import pluggy
import pytest
from pytest import Function, TestReport
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.text import bold
from trunner.types import Status, TestResult


def extract_reason_from_report(report: TestReport) -> str:
    reason = ""
    if isinstance(report.longrepr, tuple):
        reason = report.longrepr[2].replace("Skipped: ", "")
    elif hasattr(report, "wasxfail"):
        reason = report.wasxfail
        reason = f"XFAIL{': ' if len(reason) > 0 else ''}{reason}"
    return reason.strip()


def get_status_and_style(report: TestReport) -> tuple[str, dict[str, bool]]:
    was_xfail = hasattr(report, "wasxfail")
    if report.failed and report.when != "call":
        return "ERROR", {"red": True}

    if report.passed and not was_xfail:
        return "PASSED", {"green": True}
    if report.passed and was_xfail:
        return "XPASS", {"yellow": True}
    if report.failed:
        return "FAILED", {"red": True}
    if report.skipped:
        return "XFAIL" if was_xfail else "SKIPPED", {"yellow": True}

    return "UNKNOWN", {}


class PytestLogCapturePlugin:
    def __init__(self, stream_output: bool) -> None:
        self._stream_output = stream_output
        self._screen_writer = None

    @pytest.hookimpl(trylast=True)
    def pytest_configure(self, config: Any) -> None:
        terminal = config.pluginmanager.get_plugin("terminalreporter")
        if not terminal:
            return

        if self._stream_output:
            self._screen_writer = type(terminal._tw)(sys.stdout)

        terminal._tw._file = open(os.devnull, "w")

        config.option.disable_warnings = True
        config.option.no_summary = True
        terminal.summary_stats = lambda: None

    @pytest.hookimpl(tryfirst=True)
    def pytest_runtest_logstart(self, nodeid: str) -> None:
        """Manually print start header to screen."""
        if self._screen_writer:
            self._screen_writer.write(f"{nodeid}...")
            self._screen_writer.write("\n")

    @pytest.hookimpl(tryfirst=True)
    def pytest_runtest_logreport(self, report: TestReport) -> None:
        """Manually print status to screen."""
        should_print = (report.when == "call") or (not report.passed)

        if not should_print or self._screen_writer is None:
            return

        status, markup = get_status_and_style(report)
        reason = extract_reason_from_report(report)

        self._screen_writer.write(f"{report.nodeid} ")
        self._screen_writer.write(status, **markup)

        if reason:
            self._screen_writer.write(f" ({reason})")

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
        self._broken_fixtures = set()

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

        raw_before = getattr(self._dut, "before", "")
        raw_before = "" if raw_before is None else raw_before

        before = raw_before.decode("utf-8", "ignore") if isinstance(raw_before, bytes) else str(raw_before)
        before = before.replace("\r", "")

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

    bridge_plugin = PytestBridgePlugin(dut, ctx, result, kwargs)
    log_plugin = PytestLogCapturePlugin(ctx.stream_output)

    cmd_args = [str(test_path), "-s"]

    try:
        exit_code = pytest.main(cmd_args, plugins=[bridge_plugin, log_plugin])
    except Exception:
        result.fail_unknown_exception()
        return result

    if any(sub.status == Status.FAIL for sub in result.subresults):
        result.status = Status.FAIL
    elif all(sub.status == Status.SKIP for sub in result.subresults) and result.subresults:
        result.status = Status.SKIP
    elif not result.subresults or exit_code != 0:
        result.status = Status.FAIL
        result.msg = f"Pytest execution failed (Exit Code {exit_code})."
    else:
        result.status = Status.OK

    return result
