import os
import shutil
import sys
from io import StringIO
from pathlib import Path
from typing import List, Sequence

from trunner.config import ConfigParser
from trunner.dut import Dut
from trunner.harness import HarnessError, FlashError
from trunner.text import bold, green, red, yellow
from trunner.types import Status, TestOptions, TestResult, is_github_actions


def _add_tests_module_to_syspath(project_path: Path):
    # Add phoenix-rtos-tests to python path to make sure that module is visible for tests, whenever they are
    sys.path.insert(0, str(project_path / Path("phoenix-rtos-tests")))


def resolve_project_path():
    file_path = Path(__file__).resolve()
    # file_path is phoenix-rtos-project/phoenix-rtos-tests/trunner/test_runner.py
    project_dir = file_path.parent.parent.parent
    return project_dir


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


def set_logfiles(dut: Dut, logdir: str):
    """Sets dut logfiles associated with the pexpect process if needed."""

    if not logdir and not is_github_actions():
        return

    logfile_r, logfile_w, logfile_a = StringIO(""), StringIO(""), StringIO("")
    dut.set_logfiles(logfile_r, logfile_w, logfile_a)


def format_logs_for_gh(logs):
    """Prepares drop-down list for github actions workflows based on provided raw logs"""

    # skip esc codes intended to clear window and double cr to avoid printing additional newline
    logs = logs.replace("\r\r", "\r")
    logs = logs.replace("\033[2J", "")
    logs = logs.replace("\033c", "")
    logs = "::group::show logs\n" + logs + "\n::endgroup::"

    return logs


def dump_logfiles(dut: Dut, dirname: str, logdir: str):
    """Dump logfiles to log directory or to gh actions if needed."""

    if not logdir and not is_github_actions():
        return

    # we want to dump logs in the given directory
    if logdir:
        for logfile_name, log in zip(("out", "in", "inout"), dut.get_logfiles()):
            logs = log.getvalue()
            # empty string -> do not dump logs
            if not logs:
                return
            if not os.path.isdir(f"{logdir}/{dirname}"):
                os.mkdir(f"{logdir}/{dirname}")
            with open(f"{logdir}/{dirname}/{logfile_name}.log", "w") as logfile:
                logfile.write(logs)
            with open(f"{logdir}/test_campaign/{logfile_name}.log", "a") as logfile:
                logfile.write(logs)
    # we want to dump logs on Github Actions workflow
    if is_github_actions():
        logfiles = dut.get_logfiles()
        # use only out logfile
        logs = logfiles[0].getvalue()
        print(format_logs_for_gh(logs))


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

    def flash(self):
        """Flashes the device under test."""

        print("Flashing an image to device...")

        try:
            self.target.flash_dut()
        except (FlashError, HarnessError) as e:
            print(bold("ERROR WHILE FLASHING THE DEVICE"))
            print(e)
            sys.exit(1)

        print("Done!")

    def run_tests(self, tests: Sequence[TestOptions]):
        """It builds and runs tests based on given test options.

        For each test description in tests this method builds the test, runs it and prints the result.
        Additionally, reboot strategy is chosen based on the result of previous executed tests.

        Arguments:
            tests: Sequence of test options that describe how test looks like.
        """

        fail, skip = 0, 0
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
            print(f"{result.get_name()}: ", end="", flush=True)

            if test.ignore:
                result.skip()
            else:
                set_logfiles(self.target.dut, self.ctx.logdir)
                harness = self.target.build_test(test)
                assert harness is not None

                try:
                    result = harness()
                except HarnessError as e:
                    result.fail(str(e))

                if result is None:
                    # Returned type of harness is None, reinit result with default
                    result = TestResult(test.name, status=Status.OK)

            print(result, end="", flush=True)

            if result.is_fail():
                last_test_failed = True
                fail += 1
            elif result.is_ok():
                last_test_failed = False
            elif result.is_skip():
                skip += 1

            if not result.is_skip():
                testname_stripped = test.name.replace("phoenix-rtos-tests/", "").replace("/", ".")
                dump_logfiles(self.target.dut, testname_stripped, self.ctx.logdir)

        return fail, skip

    def run(self) -> bool:
        """Runs the entire test campaign based on yamls given in test_paths attribute.

        Returns true if there are no failed tests.
        """

        tests = self.parse_tests()

        init_logdir(self.ctx.logdir)

        if self.ctx.should_flash:
            set_logfiles(self.target.dut, self.ctx.logdir)
            self.flash()
            dump_logfiles(self.target.dut, "flash", self.ctx.logdir)

        if not self.ctx.should_test:
            return True

        _add_tests_module_to_syspath(self.ctx.project_path)

        fails, skips = self.run_tests(tests)
        passes = len(tests) - fails - skips

        print(f"TESTS: {len(tests)} {green('PASSED')}: {passes} {red('FAILED')}: {fails} {yellow('SKIPPED')}: {skips}")

        return fails == 0
