import importlib.util
import logging
import re

import pexpect

from .harness import UnitTestHarness, UnitTestResult
from .tools.color import Color


class TestCase:
    """A base class representing the test case loaded from YAML config"""

    PASSED = "PASSED"
    FAILED = "FAILED"
    FAILED_TIMEOUT = "FAILED TIMEOUT"
    SKIPPED = "SKIPPED"

    def __init__(self, name, target, timeout, exec_bin=None, status=None):
        self.name = name
        self.target = target
        self.timeout = timeout
        self.exec_bin = exec_bin
        if not status:
            status = TestCase.FAILED
        self.status = status
        self.harness = None
        self.exception = ''

    def skipped(self):
        return self.status == TestCase.SKIPPED

    def failed(self):
        return self.status in (TestCase.FAILED, TestCase.FAILED_TIMEOUT)

    def passed(self):
        return self.status == TestCase.PASSED

    def colored_status(self):
        if self.passed():
            color = Color.OK
        elif self.failed():
            color = Color.FAIL
        elif self.skipped():
            color = Color.SKIP

        return Color.colorify(self.status, color)

    def log_test_started(self):
        logging.info(f"{self.target}: {self.name}: ")

    def log_test_status(self):
        logging.info(self.colored_status() + '\n')
        if self.exception:
            exc_msg = ''
            for line in self.exception.splitlines():
                line = line.replace('\r', '')
                exc_msg += f'\t{line}\n'
            logging.info(exc_msg)

    def exec(self, proc):
        if not self.exec_bin:
            return

        proc.expect_exact('(psh)% ')
        proc.sendline(f'/bin/{self.exec_bin}')
        proc.expect(f'/bin/{self.exec_bin}(.*)\n')

    def handle_error(self, proc, exc):
        self.status = TestCase.FAILED_TIMEOUT

        # Look for searched pattern
        # pexpect searcher object is cleared when timeout exception is raised
        # parse patterns directly from the exception message
        r_searched = r"[\d+]: (?:re.compile\()?'(.*)'\)?"
        searched_patterns = re.findall(r_searched, exc.value)

        self.exception += 'Expected:\n'
        for idx, pattern in enumerate(searched_patterns):
            self.exception += f'\t{idx}: {pattern}\n'

        got = proc.before.replace("\r", "")
        self.exception += f'Got:\n{got}\n'

    def handle(self, proc, psh=True):
        if self.skipped():
            return True

        self.status = TestCase.FAILED

        if not self.harness:
            return False

        if psh and self.exec_bin:
            try:
                self.exec(proc)
            except pexpect.exceptions.TIMEOUT as exc:
                logging.info(f"{self.target}: Executing binary {self.exec_bin} failed!\n")
                logging.debug(f"{exc}\n")
                self.status = TestCase.FAILED_TIMEOUT
                return False

        res = False
        try:
            res = self.harness(proc)
        except pexpect.exceptions.TIMEOUT as exc:
            self.exception += 'Exception TIMEOUT\n'
            self.handle_error(proc, exc)
        except pexpect.exceptions.EOF as exc:
            self.exception += 'Exception EOF\n'
            self.handle_error(proc, exc)
        except Exception as exc:
            logging.warning(f"Test {self.name} exception:\n {exc}\n")
            raise exc

        if res:
            self.status = TestCase.PASSED

        return res


class TestCaseCustomHarness(TestCase):
    """The test case with user harness loaded from .py file"""

    def __init__(self, name, target, timeout, harness_path, exec_bin=None, status=TestCase.FAILED):
        super().__init__(name, target, timeout, exec_bin, status)
        self.load_module(harness_path)

    def load_module(self, path):
        self.spec = importlib.util.spec_from_file_location('harness', path.absolute())
        self.test_module = importlib.util.module_from_spec(self.spec)
        self.spec.loader.exec_module(self.test_module)

        if hasattr(self.test_module, 'harness'):
            self.harness = self.test_module.harness
        else:
            raise ValueError(f"harness function has not been found in {path}")


class TestCaseUnit(TestCase):
    """The test case representing Unity unit tests."""

    def __init__(self, name, target, timeout, exec_bin, status=TestCase.FAILED):
        super().__init__(name, target, timeout, exec_bin, status)
        self.harness = UnitTestHarness.harness
        self.unit_test_results = []

    def log_test_status(self):
        super().log_test_status()

        for test in self.unit_test_results:
            if test.status == UnitTestResult.FAIL:
                logging.info(f"\t{test}\n")
            else:
                logging.debug(f"\t{test}\n")

    def handle(self, proc, psh=True):
        res = super().handle(proc, psh)

        if self.status == TestCase.PASSED:
            self.unit_test_results = res

        for test in self.unit_test_results:
            if test.status == UnitTestResult.FAIL:
                self.status = TestCase.FAILED
                break

        return res


class TestCaseFactory:
    """Class responsible for creating TestCase based on a config loaded from YAML"""

    @staticmethod
    def create(test):
        status = TestCase.SKIPPED if test['ignore'] else TestCase.FAILED

        if test['type'] == 'unit':
            return TestCaseUnit(
                    name=test['name'],
                    target=test['target'],
                    timeout=test['timeout'],
                    exec_bin=test['exec'],
                    status=status
            )
        if test['type'] == 'harness':
            return TestCaseCustomHarness(
                    name=test['name'],
                    target=test['target'],
                    timeout=test['timeout'],
                    harness_path=test['harness'],
                    exec_bin=test['exec'],
                    status=status
            )

        raise ValueError(f"Unknown TestCase type: {test['type']}")
