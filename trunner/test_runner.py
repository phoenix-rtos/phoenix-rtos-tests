import logging

from .builder import TargetBuilder
from .config import TestCaseConfig, ParserArgs, set_current_target
from .device import RunnerFactory
from .testcase import TestCaseFactory
from .runners.common import Runner


class TestsRunner:
    """Class responsible for loading, building and running tests"""

    def __init__(self, target, test_paths, serial, build=True, flash=True, log=False, long_test=False):
        self.target = target
        self.test_configs = []
        self.test_paths = test_paths
        self.build = build
        self.flash = flash
        self.long_test = long_test
        self.runners = None
        self.serial = serial
        self.log = log

    def search_for_tests(self):
        paths = []
        for path in self.test_paths:
            candidate = list(path.rglob('test*.yaml'))
            if not candidate:
                raise ValueError(f'Test {path} does not contain .yaml test configuration')
            paths.extend(candidate)

        self.test_paths = paths

    def parse_tests(self):
        self.test_configs = []
        for path in self.test_paths:
            args = ParserArgs(yaml_path=path, target=self.target, long_test=self.long_test)
            config = TestCaseConfig.from_yaml(args)
            self.test_configs.extend(config.tests)
            logging.debug(f"File {path} parsed successfuly\n")

    def run(self):
        tests = []

        self.runner = RunnerFactory.create(
            target=self.target,
            serial=self.serial,
            log=self.log
            )

        set_current_target(self.target)
        self.search_for_tests()
        self.parse_tests()

        for test_config in self.test_configs:
            test = TestCaseFactory.create(test_config)
            tests.append(test)

        # deprecated utility, which will be removed soon
        if self.build:
            TargetBuilder(self.target).build()

        if self.flash:
            self.runner.flash()

        for test_case in tests:
            test_case.log_test_started()
            self.runner.run(test_case)
            test_case.log_test_status()

        passed, failed, skipped = 0, 0, 0

        # Convert bools to int
        for test in tests:
            passed += int(test.passed())
            failed += int(test.failed())
            skipped += int(test.skipped())

        if failed > 0:
            self.runner.set_status(Runner.FAIL)
        else:
            self.runner.set_status(Runner.SUCCESS)

        return passed, failed, skipped
