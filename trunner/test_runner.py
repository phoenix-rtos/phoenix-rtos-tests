import sys
from typing import Sequence

from trunner.config import ConfigParser
from trunner.harness import HarnessError, FlashError
from trunner.text import bold, green, red, yellow
from trunner.types import TestOptions, TestResult


class TestRunner:
    """Class responsible for loading, building and running tests"""

    def __init__(self, ctx, test_paths):
        self.ctx = ctx
        self.target = self.ctx.target
        self.test_configs = []
        self.test_paths = test_paths

    def search_for_tests(self):
        paths = []
        for path in self.test_paths:
            candidate = list(path.rglob("test*.yaml"))
            if not candidate:
                raise ValueError(f"Test {path} does not contain .yaml test configuration")
            paths.extend(candidate)

        self.test_paths = paths

    def parse_tests(self):
        self.search_for_tests()
        parser = ConfigParser(self.ctx)

        tests = []
        for path in self.test_paths:
            tests.extend(parser.parse(path))

        return tests

    def flash(self):
        try:
            self.target.flash_dut()
        except (FlashError, HarnessError) as e:
            print(bold("ERROR WHILE FLASHING THE DEVICE"))
            print(e)
            sys.exit(1)

    def run_tests(self, tests: Sequence[TestOptions]):
        fail, skip = 0, 0

        for idx, test in enumerate(tests):
            result = TestResult(test.name)
            print(f"{result.get_name()}: ", end="", flush=True)

            if test.ignore:
                result.skip()
            else:
                harness = self.target.build_test(test)
                assert harness is not None

                res = None
                try:
                    res = harness()
                except HarnessError as e:
                    result.fail(str(e))

                if res is not None:
                    result = res

            print(result, end="", flush=True)

            if result.is_fail():
                fail += 1
            elif result.is_skip():
                skip += 1

            def set_reboot_flag(tests, idx, result):
                # If the test is successful and the next test doesn't require loading via
                # the plo we don't want to reboot the entire device (to speed up the test execution).
                # There are three exceptions to this rule:
                # 1. Runner runs in the "nightly" mode when we are not concerned about slow execution.
                # 2. The test has failed.
                # 3. We have to enter the bootloader in order to load applications.
                if idx == len(tests) - 1:
                    return

                if result.is_skip():
                    tests[idx + 1].should_reboot = tests[idx].should_reboot

                if (
                    result.is_fail()
                    or self.ctx.nightly
                    or (tests[idx + 1].bootloader is not None and tests[idx + 1].bootloader.apps)
                ):
                    tests[idx + 1].should_reboot = True
                else:
                    tests[idx + 1].should_reboot = False

            set_reboot_flag(tests, idx, result)

        return fail, skip

    def run(self):
        tests = self.parse_tests()

        if self.ctx.should_flash:
            self.flash()

        if not self.ctx.should_test:
            return True

        fails, skips = self.run_tests(tests)
        oks = len(tests) - fails - skips

        print(f"TESTS: {len(tests)} {green('PASSED')}: {oks} {red('FAILED')}: {fails} {yellow('SKIPPED')}: {skips}")

        return fails == 0
