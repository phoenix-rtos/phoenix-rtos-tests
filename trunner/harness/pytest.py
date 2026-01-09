import io
import pytest
from trunner.dut import Dut
from trunner.ctx import TestContext
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
        if terminal and self._suppress:
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

    @pytest.fixture
    def dut(self):
        return self._dut

    @pytest.fixture
    def ctx(self):
        return self._ctx

    @pytest.fixture
    def kwargs(self):
        return self._kwargs

    @pytest.hookimpl(specname="pytest_runtest_logreport")
    def pytest_trunner_hook(self, report):
        """PyTest hook called after a test phase completion.

        Adds a TestRunner subresult for each PyTest case report

        :param report: PyTest's TestReport object
        """

        # We want to capture only the test call or setup failures (avoids a triple report)
        is_call_stage = (report.when == "call")
        is_setup_failure = (report.when == "setup" and (report.failed or report.skipped))

        if not (is_call_stage or is_setup_failure):
            return
        
        status = Status.OK

        if report.failed:
            status = Status.FAIL
        elif report.skipped and not hasattr(report, "wasxfail"):
            status = Status.SKIP

        self._result.add_subresult(
            subname=report.nodeid.split("::")[-1], 
            status=status, 
            msg=""
        )


def pytest_harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> TestResult:
    if "path" not in kwargs:
        result.fail(msg="missing `path` from test configuration!")
        return result

    test_path = ctx.project_path / kwargs["path"]
    options = kwargs.get("options", "").split()
    
    bridge_plugin = PytestBridgePlugin(dut, ctx, result, kwargs)
    log_plugin = PytestLogCapturePlugin(ctx.stream_output)

    cmd_args = [
        str(test_path),
        *options,
        "-v",
        "-s",
        "--tb=no",
    ]

    try:
        exit_code = pytest.main(cmd_args, plugins=[bridge_plugin, log_plugin])
    except Exception as e:
        result.fail_harness_exception(e)
        return result

    if any(sub.status == Status.FAIL for sub in result.subresults):
        result.status = Status.FAIL
    elif all(sub.status == Status.SKIP for sub in result.subresults) and result.subresults:
        result.status = Status.SKIP
    elif not result.subresults and exit_code != 0:
        result.status = Status.FAIL
        result.msg = f"Pytest execution failed (Exit Code {exit_code})."
    else:
        result.status = Status.OK

    if result.status == Status.FAIL:
        captured_log = log_plugin.get_logs()
        result.msg += captured_log

    return result
