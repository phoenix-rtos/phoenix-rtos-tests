import os
import re
import shlex
import sys
from collections.abc import Generator
from io import TextIOWrapper
from typing import Any

import pexpect
import pytest
from pytest import Function, TestReport
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.text import bold, green, red, yellow
from trunner.types import Status, TestResult


def extract_reason_from_report(report: TestReport) -> str:
    """Extracts and formats the skip or xfail reason from a test report"""
    reason = ""
    if isinstance(report.longrepr, tuple):
        reason = report.longrepr[2].replace("Skipped: ", "")
    elif hasattr(report, "wasxfail"):
        reason = report.wasxfail
        reason = f"XFAIL: {reason}" if reason else "XFAIL"
    return reason.strip()


def get_status(report: TestReport) -> str:
    """Converts a PyTest report status into a formatted trunner status string"""
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


def get_str_attr(obj: object, attribute: str) -> str:
    """Safely retrieves an object attribute and ensures it is returned as a clean string"""
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
        self._sink: TextIOWrapper | None = None
        self._last_handled_state: tuple[Any, Any, Any] | None = None

    @pytest.hookimpl(trylast=True)
    def pytest_configure(self, config: Any) -> None:
        terminal = config.pluginmanager.get_plugin("terminalreporter")
        if not terminal:
            return

        if self._ctx.stream_output:
            self._screen_writer = type(terminal._tw)(sys.stdout)

        self._sink = open(os.devnull, "w")
        terminal._tw._file = self._sink

        config.option.disable_warnings = True
        config.option.no_summary = True
        terminal.summary_stats = lambda: None

    @pytest.hookimpl(trylast=True)
    def pytest_unconfigure(self, config: Any) -> None:
        if self._sink is not None:
            self._sink.close()
            self._sink = None

    @pytest.hookimpl(tryfirst=True)
    def pytest_runtest_logstart(self, nodeid: str) -> None:
        """Manually print start header to screen"""
        if self._screen_writer is None:
            return

        self._flush_and_ensure_newline()
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

        self._flush_and_ensure_newline()

        self._screen_writer.write(f"\t{bold(report.nodeid)} ")
        self._screen_writer.write(status)

        if reason:
            self._screen_writer.write(f" ({reason})")

        self._screen_writer.write("\n")

    def _flush_and_get_last_char(self, pexpect_p: Any) -> str:
        drained_last_char = ""
        try:
            while True:
                chunk = pexpect_p.read_nonblocking(size=4096, timeout=0)
                if not chunk:
                    break

                if isinstance(chunk, bytes):
                    drained_last_char = chunk[-1:].decode("utf-8", "ignore")
                else:
                    drained_last_char = chunk[-1:]

                # NOTE: The buffer appending operations below do not cause logfile duplication/
                # The data is manually placed into the internal pexpect buffer, subsequent expect()
                # calls omit the OS buffer pull operation that triggers the logfile write.
                if isinstance(pexpect_p.buffer, bytes) and isinstance(chunk, str):
                    pexpect_p.buffer += chunk.encode("utf-8", "ignore")
                elif isinstance(pexpect_p.buffer, str) and isinstance(chunk, bytes):
                    pexpect_p.buffer += chunk.decode("utf-8", "ignore")
                else:
                    pexpect_p.buffer += chunk
        except (pexpect.TIMEOUT, pexpect.EOF):
            pass

        return drained_last_char

    def _flush_and_ensure_newline(self) -> None:
        """Flushes pending OS logs and ensures output ends with a newline.

        This drains pending OS logs so they print continuously, restores them
        so they can be parsed in the incoming test, and checks the absolute last character
        to ensure the output ends with a newline for Pytest headers to format correctly.
        """
        dut = getattr(self._ctx.target, "dut", None)
        pexpect_p = getattr(dut, "pexpect_proc", None)
        if dut is None or pexpect_p is None:
            return

        drained_last_char = self._flush_and_get_last_char(pexpect_p)
        last_char = ""

        if drained_last_char:
            last_char = drained_last_char
            self._last_handled_state = (
                getattr(pexpect_p, "before", ""),
                getattr(pexpect_p, "after", ""),
                getattr(pexpect_p, "buffer", ""),
            )
        else:
            before = getattr(pexpect_p, "before", "")
            after = getattr(pexpect_p, "after", "")
            buffer = getattr(pexpect_p, "buffer", "")

            current_state = (before, after, buffer)

            if self._last_handled_state == current_state:
                return
            self._last_handled_state = current_state

            raw_char = None

            if buffer:
                raw_char = buffer[-1:]
            elif isinstance(after, (str, bytes)) and after:
                raw_char = after[-1:]
            elif isinstance(before, (str, bytes)) and before:
                raw_char = before[-1:]

            if isinstance(raw_char, bytes):
                last_char = raw_char.decode("utf-8", "ignore")
            elif raw_char is not None:
                last_char = raw_char

        if self._screen_writer and last_char and last_char not in ("\n", "\r"):
            self._screen_writer.write("\n")


class PytestBridgePlugin:
    """Plugin that bridges the TestRunner and PyTest frameworks.

    Each test case is run as TestRunner's subtest, accurately
    capturing each subresult's runtime and status.
    """

    _unknown_fixture_str = "[UNKNOWN FIXTURE]"
    _common_fast_fail_str = "Requires a previously failing fixture:"
    _fast_fail_match_re = re.compile(rf"{re.escape(_common_fast_fail_str)} (\w+)")
    _decorator_match_re = re.compile(r"@pytest\.fixture.*?def\s+(\w+)", re.DOTALL)

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

    @pytest.hookimpl(wrapper=True)
    def pytest_runtest_makereport(self, item: Function, call: pytest.CallInfo) -> Generator[None, Any, TestReport]:
        """Post-process PyTest's report making

        Inject output caught before exception for a more detailed report.

        Currently, TestResult failing methods fail the entire test with the
        most recent exception, with no regard to individual subresults.

        TODO: Modify TestResult failing methods to support single subresult
        exceptions and use them here to avoid reimplementation.
        """
        report: TestReport = yield

        if not report.failed or not call.excinfo:
            return report

        exc_instance = call.excinfo.value
        report_add_info = None

        before = get_str_attr(self._dut, "before")

        if isinstance(exc_instance, (UnicodeDecodeError, pexpect.TIMEOUT, pexpect.EOF)) and before:
            report_add_info = bold("OUTPUT CAUGHT BEFORE EXCEPTION: ")
            report_add_info += before

        if report_add_info:
            report.longrepr = str(report.longrepr) + "\n\n" + report_add_info
        return report

    @pytest.hookimpl()
    def pytest_runtest_logreport(self, report: TestReport) -> None:
        """PyTest hook called after a test phase completion.

        Adds a TestRunner subresult for each PyTest case report

        :param report: PyTest's TestReport object
        """
        error_msg = getattr(report, "longreprtext", str(report.longrepr)) if report.longrepr else ""

        if report.when == "setup":
            self._record_setup_phase(report, error_msg)

        elif report.when == "call":
            self._record_call_phase(report, error_msg)

        elif report.when == "teardown":
            self._finalize_and_report(report, error_msg)

    def _record_setup_phase(self, report: TestReport, error_msg: str) -> None:
        if report.failed:
            fixture_name = self._extract_broken_fixture(error_msg)
            self._pending_result = (Status.FAIL, error_msg)
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
        final_status, final_msg = self._pending_result

        if report.failed:
            final_status = Status.FAIL

            if final_msg:
                final_msg = f"{final_msg}\n{error_msg}"
            else:
                final_msg = error_msg

        self._result.add_subresult(subname=report.nodeid.split("::")[-1], status=final_status, msg=final_msg)

    @classmethod
    def _extract_broken_fixture(cls, text: str) -> str:
        fast_fail_match = cls._fast_fail_match_re.search(text)
        if fast_fail_match:
            return fast_fail_match.group(1)
        decorator_match = cls._decorator_match_re.search(text)
        if decorator_match:
            return decorator_match.group(1)
        return cls._unknown_fixture_str


def pytest_harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> TestResult:
    test_path = kwargs.get("script")

    if not test_path:
        result.fail(msg="missing `script` from test configuration!")
        return result

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
        result.fail(msg=f"Pytest execution failed (Exit Code {exit_code}).")

    return result
