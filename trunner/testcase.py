import importlib.util
import logging
import re
import shlex
import sys
import traceback

from pexpect.exceptions import TIMEOUT, EOF

from .harnesses.factory import TestHarnessFactory

from .tools.color import Color
from .config import SYSEXEC_TARGETS, LONG_TESTS


class TestCase:
    """A base class representing the test case loaded from YAML config"""

    PASSED = "PASSED"
    FAILED = "FAILED"
    FAILED_TIMEOUT = "FAILED TIMEOUT"
    SKIPPED = "SKIPPED"

    def __init__(
        self,
        name,
        target,
        timeout,
        psh=True,
        exec_cmd=None,
        use_sysexec=False,
        status=None
    ):
        self.name = name
        self.target = target
        self.timeout = timeout
        self.exec_cmd = exec_cmd
        self.psh = psh
        self.use_sysexec = use_sysexec
        if not status:
            status = TestCase.FAILED
        self.status = status
        self.harness = None
        self.exception = ''

    def fail(self):
        self.status = TestCase.FAILED

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
        cmd = ' '.join(shlex.quote(arg) for arg in self.exec_cmd)
        proc.sendline(f'/bin/{cmd}')
        proc.expect(f'/bin/{cmd}(.*)\n')

    def sysexec(self, proc):
        cmd = ' '.join(shlex.quote(arg) for arg in self.exec_cmd)
        proc.sendline(f'sysexec {cmd}')
        proc.expect(f'sysexec {cmd}(.*)\n')

    def exec_test(self, proc):
        try:
            # Wait for a prompt
            # On armv7a9-zynq7000-qemu jffs initialization may take to ~20s, that's why it's set to 25
            if self.target == "armv7a9-zynq7000-qemu":
                proc.expect_exact('(psh)% ', timeout=25)
            else:
                proc.expect_exact('(psh)% ')
            if self.exec_cmd:
                if self.use_sysexec:
                    self.sysexec(proc)
                else:
                    self.exec(proc)
        except (TIMEOUT, EOF) as exc:
            msg = "Execution of test binary failed!\n"
            self.exception = Color.colorify(msg, Color.BOLD)
            self.handle_pyexpect_error(proc, exc)
            return
        except UnicodeDecodeError as exc:
            self.handle_unicode_error(proc, exc)
            return

    def handle_pyexpect_error(self, proc, exc):
        self.status = TestCase.FAILED_TIMEOUT

        # Look for searched pattern
        # pexpect searcher object is cleared when timeout exception is raised
        # parse patterns directly from the exception message
        r_searched = r"[\d+]: (?:re.compile\()?b?['\"](.*)['\"]\)?"
        searched_patterns = re.findall(r_searched, exc.value)

        self.exception += Color.colorify('EXPECTED:\n', Color.BOLD)
        for idx, pattern in enumerate(searched_patterns):
            self.exception += f'\t{idx}: {pattern}\n'

        got = str(proc.before)
        got = got.replace("\r", "")
        self.exception += Color.colorify('GOT:\n', Color.BOLD)
        self.exception += f'{got}\n'

    def handle_unicode_error(self, proc, exc):
        self.status = TestCase.FAILED
        msg = 'Unicode Decode Error detected!\n'
        self.exception = Color.colorify(msg, Color.BOLD)
        self.exception += Color.colorify('STRING WITH NON-ASCII CHARACTER:\n', Color.BOLD)
        self.exception += f'{exc.object}\n'
        self.exception += Color.colorify('OUTPUT CAUGHT BEFORE EXCEPTION:\n', Color.BOLD)
        got = str(proc.before)
        got = got.replace("\r", "")
        self.exception += f'{got}\n'

    def handle_assertion(self, proc, exc):
        self.status = TestCase.FAILED

        _, _, exc_traceback = sys.exc_info()
        # Get rid off "self.harness()" call info
        tb_info = traceback.format_tb(exc_traceback)[1:]

        self.exception += Color.colorify(
                'ASSERTION TRACEBACK (most recent call last):\n',
                Color.BOLD
        )
        self.exception += ''.join(tb_info)

        if exc.args:
            self.exception += Color.colorify('ASSERTION MESSAGE:\n', Color.BOLD)
            self.exception += f'{exc}\n'

        if proc and proc.buffer:
            got = proc.buffer.replace("\r", "")
            self.exception += Color.colorify('READER BUFFER:\n', Color.BOLD)
            self.exception += f'{got}\n'

    def handle_exception(self):
        self.exception = Color.colorify('EXCEPTION:\n', Color.BOLD)
        self.exception += traceback.format_exc()
        self.status = TestCase.FAILED

    def handle(self, proc):
        if self.skipped():
            return

        self.status = TestCase.PASSED

        if self.psh:
            self.exec_test(proc)
            if self.failed():
                return

        res = None
        try:
            res = self.harness(proc)
        except (TIMEOUT, EOF) as exc:
            msg = 'EXCEPTION ' + ('EOF' if isinstance(exc, EOF) else 'TIMEOUT') + '\n'
            self.exception += Color.colorify(msg, Color.BOLD)
            self.handle_pyexpect_error(proc, exc)
        except UnicodeDecodeError as exc:
            self.handle_unicode_error(proc, exc)
            return
        except AssertionError as exc:
            self.handle_assertion(proc, exc)
        except Exception:
            self.handle_exception()

        return res


class TestCaseCustomHarness(TestCase):
    """The test case with user harness loaded from .py file"""

    def __init__(
        self,
        name,
        target,
        timeout,
        harness_path,
        psh=True,
        exec_cmd=None,
        use_sysexec=False,
        status=TestCase.FAILED
    ):
        super().__init__(name, target, timeout, psh, exec_cmd, use_sysexec, status)
        self.load_module(harness_path)

    def load_module(self, path):
        self.spec = importlib.util.spec_from_file_location('harness', path.absolute())
        self.test_module = importlib.util.module_from_spec(self.spec)
        self.spec.loader.exec_module(self.test_module)

        if hasattr(self.test_module, 'harness'):
            self.harness = self.test_module.harness
        else:
            raise ValueError(f"harness function has not been found in {path}")


class TestCaseThirdParty(TestCase):
    """The test case representing tests run by 3rd party frameworks """

    def __init__(
        self,
        type,
        name,
        target,
        timeout,
        exec_cmd,
        psh=True,
        use_sysexec=False,
        status=TestCase.FAILED
    ):
        super().__init__(name, target, timeout, psh, exec_cmd, use_sysexec, status)
        self.harness = TestHarnessFactory.create(type)
        self.test_results = []

    def log_test_status(self):
        super().log_test_status()

        for test in self.test_results:
            if test.status == test.FAIL:
                logging.info(f"\t{test}\n")
            else:
                logging.debug(f"\t{test}\n")

    def handle(self, proc):
        res = super().handle(proc)

        if self.status == self.PASSED:
            self.test_results = res

        for test in self.test_results:
            if test.status == test.FAIL:
                self.status = self.FAILED
                break

        return res


class TestCaseFactory:
    """Class responsible for creating TestCase based on a config loaded from YAML"""

    @staticmethod
    def create(test):
        status = TestCase.SKIPPED if test['ignore'] else TestCase.FAILED
        use_sysexec = test['target'] in SYSEXEC_TARGETS

        if test['type'] == 'unit' or test['type'] in LONG_TESTS:
            return TestCaseThirdParty(
                type=test['type'],
                name=test['name'],
                target=test['target'],
                timeout=test['timeout'],
                psh=test['psh'],
                exec_cmd=test.get('exec'),
                use_sysexec=use_sysexec,
                status=status
            )
        if test['type'] == 'harness':
            return TestCaseCustomHarness(
                name=test['name'],
                target=test['target'],
                timeout=test['timeout'],
                psh=test['psh'],
                harness_path=test['harness'],
                exec_cmd=test.get('exec'),
                use_sysexec=use_sysexec,
                status=status
            )

        raise ValueError(f"Unknown TestCase type: {test['type']}")
