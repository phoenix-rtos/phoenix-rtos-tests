import os
import shutil
import sys
import junitparser
from io import StringIO
from pathlib import Path
from collections import Counter
from typing import List, Sequence, TextIO

from trunner.config import ConfigParser
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.harness import HarnessError, FlashError
from trunner.text import bold, green, red, yellow, magenta
from trunner.types import Status, TestOptions, TestResult, TestStage, is_github_actions, get_ci_url


def _add_tests_module_to_syspath(project_path: Path):
    # Add phoenix-rtos-tests to python path to make sure that module is visible for tests, whenever they are
    sys.path.insert(0, str(project_path / Path("phoenix-rtos-tests")))


def resolve_project_path():
    file_path = Path(__file__).resolve()
    # file_path is phoenix-rtos-project/phoenix-rtos-tests/trunner/test_runner.py
    project_dir = file_path.parent.parent.parent
    return project_dir


class LogWrapper(StringIO):
    """Wrapper for saving all logs into StringIO and also streaming directly to stream"""

    def __init__(self, stream: TextIO):
        super().__init__("")
        self.stream = stream

    def write(self, message):
        # skip esc codes intended to clear window when streaming logs
        stripped_message = message.replace("\033[2J", "").replace("\033c", "")
        self.stream.write(stripped_message)

        return super().write(message)

    def flush(self):
        self.stream.flush()
        return super().flush()


def init_logdir(logdir: str):
    """Inits log directories if it's needed."""
    if not logdir:
        return

    # clear the whole log directory before the next campaign
    if os.path.isdir(logdir):
        for file in os.listdir(logdir):
            shutil.rmtree(f"{logdir}/{file}")

    # test campaign directory will always be needed
    os.makedirs(f"{logdir}/test_campaign")


def set_logfiles(dut: Dut, ctx: TestContext):
    """Sets dut logfiles associated with the pexpect process if needed."""

    if not ctx.logdir and not ctx.stream_output:
        return

    logfile_r = StringIO()
    if ctx.stream_output:
        logfile_r = LogWrapper(sys.stdout)

    logfile_w, logfile_a = StringIO(""), StringIO("")
    dut.set_logfiles(logfile_r, logfile_w, logfile_a)


def save_logfiles(dut: Dut, dirname: str, logdir: str):
    """Save logfiles to log directory if needed."""

    # we want to dump logs in the given directory
    if logdir:
        for logfile_name, log in zip(("out", "in", "inout"), dut.get_logfiles()):
            logs = log.getvalue()
            # empty string -> do not dump logs
            if not logs:
                return
            if not os.path.isdir(f"{logdir}/{dirname}"):
                os.mkdir(f"{logdir}/{dirname}")
            with open(f"{logdir}/{dirname}/{logfile_name}.log", "w", encoding="utf-8") as logfile:
                logfile.write(logs)
            with open(f"{logdir}/test_campaign/{logfile_name}.log", "a", encoding="utf-8") as logfile:
                logfile.write(logs)


class TestRunner:
    """Class responsible for loading, building and running tests"""

    def __init__(self, ctx: TestContext, test_paths):
        self.ctx = ctx
        self.target = self.ctx.target
        self.test_configs = []
        self.test_paths = test_paths

    def search_for_tests(self) -> List[Path]:
        """Returns test*.yaml files that are searched in directories given in test_paths attribute."""

        paths = []
        for path in self.test_paths:
            yamls = []

            if path.is_dir():
                for p in list(path.rglob("test*.yaml")) + list(path.rglob("test*.yml")):
                    if ".git" not in str(p):
                        yamls.append(p)

                yamls.sort()
                if not yamls:
                    raise ValueError(f"{path} does not contain .yaml test configuration")
            elif path.is_file():
                if path.suffix not in (".yaml", ".yml"):
                    raise ValueError("Test configuration must be a file with .yaml or .yml extension")

                yamls = [path]
            else:
                raise ValueError(f"Test configuration {path} is neither a directory nor a file.")

            paths.extend(yamls)

        return paths

    def parse_tests(self) -> Sequence[TestOptions]:
        """Returns test options that can be used to build test harness."""

        test_yamls = self.search_for_tests()
        parser = ConfigParser(self.ctx)

        tests = []
        for path in test_yamls:
            tests.extend(parser.parse(path))

        return tests

    def flash(self) -> TestResult:
        """Flashes the device under test."""

        print("Flashing an image to device...")

        # report flashing as a test result to include in export (especially useful when failed)
        result = TestResult("flash")

        try:
            result.set_stage(TestStage.RUN)
            self.target.flash_dut()
            result.set_stage(TestStage.DONE)
        except (FlashError, HarnessError) as exc:
            # the newline is needed to avoid printing exception in the same line as plo prompt
            print(bold("\nERROR WHILE FLASHING THE DEVICE"))
            print(exc)
            result.fail_harness_exception(exc)

        print("Done!")
        return result

    def _print_test_header_end(self, test: TestOptions):
        if self.ctx.stream_output:
            print("")  # add newline as test logs might have not ended with it
            if is_github_actions():
                print("::endgroup::")

            # while streaming output highlight each new test with color
            print(f"{magenta(test.name)}: ", end="", flush=True)

    def _print_test_header_begin(self, test: TestOptions):
        if self.ctx.stream_output:
            if is_github_actions():
                print("::group::", end="")

            # while streaming output highlight each new test with color
            print(f"{magenta(test.name)}: ...")
        else:
            print(f"{test.name}: ", end="", flush=True)

    def _export_results_csv(self, results: Sequence[TestResult]):
        """write results to fname in CSV format"""
        if not self.ctx.output:
            return

        fname = self.ctx.output + ".csv"

        with open(fname, "w", encoding="utf-8") as out_csv:
            out_csv.write(TestResult.get_csv_header() + "\n")
            for res in results:
                out_csv.write(res.to_csv() + "\n")

        print(f"Test results written to: {fname}")

    def _export_results_xml(self, results: Sequence[TestResult]):
        """write results to fname in jUnit XML format"""
        if not self.ctx.output:
            return

        fname = self.ctx.output + ".xml"
        # we should be able to have testsuites within testsuites but many tools doesn't support that
        # map every TestResult to TestSuite instead

        xml = junitparser.JUnitXml()
        for res in results:
            suite = res.to_junit_testsuite(self.ctx.target.name)
            suite.hostname = self.ctx.host.name
            if is_github_actions():
                suite.add_property("url", get_ci_url())
                suite.add_property("SHA", os.environ["GITHUB_SHA"])

            xml.add_testsuite(suite)

        xml.update_statistics()
        try:
            xml.write(fname, pretty=True)  # TODO: remove pretty
        except Exception:
            # DEBUG: in case of XML error - try dumping to stdout to inspect the XML
            try:
                from lxml import etree
            except ImportError:
                from xml.etree import ElementTree as etree
            with open(fname, "wb") as out_xml:
                text = etree.tostring(xml._elem)
                out_xml.write(text)

            raise

        print(f"Test results written to: {fname}")

    def run_tests(self, tests: Sequence[TestOptions]) -> Sequence[TestResult]:
        """It builds and runs tests based on given test options.

        For each test description in tests this method builds the test, runs it and prints the result.
        Additionally, reboot strategy is chosen based on the result of previous executed tests.

        Arguments:
            tests: Sequence of test options that describe how test looks like.
        """

        results = []
        # Ensure first test will start with reboot
        last_test_failed = True

        for test in tests:
            # By default we don't want to reboot the entire device to speed up the test execution)
            # if not explicitly required by the test.
            if last_test_failed:
                test.should_reboot = True

            if self.ctx.nightly:
                test.should_reboot = True

            # We have to enter the bootloader in order to load applications.
            if not self.ctx.target.rootfs and test.bootloader is not None and test.bootloader.apps:
                test.should_reboot = True

            result = TestResult(test.name)
            self._print_test_header_begin(test)

            if test.ignore:
                result.skip()
            else:
                set_logfiles(self.target.dut, self.ctx)
                harness = self.target.build_test(test)

                if not test.should_reboot:  # WARN: build_test may change TestOptions
                    # if not rebooting - force new prompt to appear
                    self.target.dut.send("\n")

                test_result = None
                assert harness is not None

                try:
                    test_result = harness(result)
                    assert test_result is not None, "harness needs to return TestResult"
                    result.overwrite(test_result)
                except HarnessError as e:
                    result.fail(str(e))

            self.target.dut.read(timeout=0.1)  # try to read (pass to logs) remaining test output
            self._print_test_header_end(test)
            print(result.to_str(self.ctx.verbosity), end="", flush=True)

            results.append(result)

            if result.is_skip():
                continue

            last_test_failed = result.is_fail()

            save_logfiles(self.target.dut, result.shortname, self.ctx.logdir)

        return results

    def run(self) -> bool:
        """Runs the entire test campaign based on yamls given in test_paths attribute.

        Returns true if there are no failed tests.
        """

        tests = self.parse_tests()

        init_logdir(self.ctx.logdir)
        results = []

        run_tests = self.ctx.should_test

        if self.ctx.should_flash:
            set_logfiles(self.target.dut, self.ctx)
            flash_result = self.flash()
            save_logfiles(self.target.dut, "flash", self.ctx.logdir)
            results.append(flash_result)

            if not flash_result.is_ok():
                run_tests = False

        if run_tests:
            _add_tests_module_to_syspath(self.ctx.project_path)
            results.extend(self.run_tests(tests))

        sums = Counter(res.status for res in results)

        print(
            f"TESTS: {len(results)} "
            f"{green('PASSED')}: {sums.get(Status.OK, 0)} "
            f"{red('FAILED')}: {sums.get(Status.FAIL, 0)} "
            f"{yellow('SKIPPED')}: {sums.get(Status.SKIP, 0)}"
        )

        self._export_results_csv(results)
        self._export_results_xml(results)

        return sums.get(Status.FAIL, 0) == 0
