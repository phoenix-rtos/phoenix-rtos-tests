import io
import re
import shlex

import pytest
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult


class PytestLogCapturePlugin:
    """Plugin for intercepting and optionally suppressing PyTest output.

    Diverts the PyTest's detailed report stream into an internal buffer
    and allows for complete suppression to keep the terminal clean.
    """
    def __init__(self, stream_output):
        self.buffer = io.StringIO()
        self._suppress = not stream_output

    @pytest.hookimpl(trylast=True)
    def pytest_configure(self, config):
        terminal = config.pluginmanager.get_plugin("terminalreporter")
        if not terminal:
            return

        # Disabling PyTest's summary completely
        # This information is handled by TRunner instead
        config.option.disable_warnings = True
        config.option.no_summary = True
        terminal.summary_stats = lambda: None

        if self._suppress:
            terminal._tw._file = self.buffer

    def get_logs(self) -> str:
        """Get the full, unsuppressed pytest log
        """
        return self.buffer.getvalue()


class PytestBridgePlugin:
    """Plugin that bridges the TestRunner and PyTest frameworks.

    Each unit test is run as a TestRunner's subtest, accurately
    capturing each subresult's runtime and status.
    """
    def __init__(self, dut, ctx, result, kwargs):
        self._dut = dut
        self._ctx = ctx
        self._kwargs = kwargs
        self._result = result
        self._test_outcomes = {}
        self._broken_fixtures = set()
        self._unknown_fixture_str = "[UNKNOWN FIXTURE]"
        self._common_fast_fail_str = "Requires a previously failing fixture:"

    @pytest.fixture
    def dut(self):
        return self._dut

    @pytest.fixture
    def ctx(self):
        return self._ctx

    @pytest.fixture
    def kwargs(self):
        return self._kwargs

    @pytest.hookimpl(tryfirst=True)
    def pytest_runtest_setup(self, item):
        for fixture in item.fixturenames:
            # If a fixture failed previously during setup, fail immediately
            if fixture in self._broken_fixtures:
                pytest.fail(f"{self._common_fast_fail_str} {fixture}", pytrace=False)

    @pytest.hookimpl()
    def pytest_runtest_logreport(self, report):
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

    def _record_setup_phase(self, report, error_msg):
        if report.failed:
            fixture_name = self._extract_broken_fixture(error_msg)
            self._test_outcomes[report.nodeid] = {
                "status": Status.FAIL,
                "msg": f"{error_msg}",  # Newline for improved readability
            }
            if fixture_name != self._unknown_fixture_str:
                # It is assumed that only a fixture that fails at startup will
                # be marked as 'broken', thus disabling all test-cases using it
                self._broken_fixtures.add(fixture_name)
            return
        if report.skipped:
            skip_msg = self._extract_reason(report)
            self._test_outcomes[report.nodeid] = {"status": Status.SKIP, "msg": skip_msg}
            return

    def _record_call_phase(self, report, error_msg):
        status = Status.OK
        msg = ""
        if report.failed:
            status = Status.FAIL
        elif report.skipped:
            status = Status.SKIP
            reason = self._extract_reason(report)
            # XFAIL is treated as SKIP to not confuse it with success but
            # it is not a FAIL either. XPASS will still be treated as OK
            if hasattr(report, "wasxfail"):
                msg = f"XFAIL{': ' if len(reason) > 0 else ''}{reason}"
            else:
                msg = reason

        self._test_outcomes[report.nodeid] = {"status": status, "msg": error_msg if report.failed else msg}
        return

    def _finalize_and_report(self, report, error_msg):
        stored = self._test_outcomes.pop(report.nodeid, {"status": Status.OK, "msg": ""})
        final_status = stored["status"]
        final_msg = stored["msg"]
        teardown_msg = error_msg

        if report.failed:
            final_status = Status.FAIL

            if final_msg:
                final_msg = f"{final_msg}\n{teardown_msg}"
            else:
                final_msg = f"{teardown_msg}"

        self._result.add_subresult(subname=report.nodeid.split("::")[-1], status=final_status, msg=final_msg)

    def _extract_reason(self, report):
        reason = ""
        if isinstance(report.longrepr, tuple):
            reason = report.longrepr[2].replace("Skipped: ", "")
        elif hasattr(report, "wasxfail"):
            reason = report.wasxfail.replace("reason: ", "")
        return reason.strip()

    def _extract_broken_fixture(self, text):
        fast_fail_match = re.search(rf"{re.escape(self._common_fast_fail_str)} (\w+)", text)
        if fast_fail_match:
            return fast_fail_match.group(1)
        decorator_match = re.search(r"@pytest\.fixture.*?def\s+(\w+)", text, re.DOTALL)
        if decorator_match:
            return decorator_match.group(1)
        return self._unknown_fixture_str


def pytest_harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> TestResult:
    if "path" not in kwargs:
        result.fail(msg="missing `path` from test configuration!")
        return result

    test_path = ctx.project_path / kwargs["path"]
    options = shlex.split(kwargs.get("options", ""))

    bridge_plugin = PytestBridgePlugin(dut, ctx, result, kwargs)
    log_plugin = PytestLogCapturePlugin(ctx.stream_output)

    cmd_args = [str(test_path), *options, "-v", "-s"]

    try:
        exit_code = pytest.main(cmd_args, plugins=[bridge_plugin, log_plugin])
    except Exception as e:
        result.fail_harness_exception(e)
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
