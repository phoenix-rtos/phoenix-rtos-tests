import logging

from . import config
from .builder import TargetBuilder
from .config import TestCaseConfig, ParserArgs
from .device import RunnerFactory
from .testcase import TestCaseFactory
from .runners.common import Runner


class TestsRunner:
    """Class responsible for loading, building and running tests"""

    def __init__(self, targets, test_paths, serial, build=True, flash=True, log=False):
        self.targets = targets
        self.test_configs = []
        self.test_paths = test_paths
        self.tests_per_target = {k: [] for k in targets}
        self.build = build
        self.flash = flash
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
            args = ParserArgs(yaml_path=path, targets=self.targets)
            config = TestCaseConfig.from_yaml(args)
            self.test_configs.extend(config.tests)
            logging.debug(f"File {path} parsed successfuly\n")

    def run(self):
        self.runners = {target: RunnerFactory.create(
            target=target,
            serial=self.serial,
            log=self.log
            ) for target in self.targets}
        self.search_for_tests()
        self.parse_tests()

        for test_config in self.test_configs:
            test = TestCaseFactory.create(test_config)
            self.tests_per_target[test.target].append(test)

        if self.build:
            for target in self.targets:
                TargetBuilder(target).build()

        if self.flash:
            for runner in self.runners.values():
                runner.flash()

        for target, tests in self.tests_per_target.items():
            config.CURRENT_TARGET = target
            for test_case in tests:
                test_case.log_test_started()
                self.runners[target].run(test_case)
                test_case.log_test_status()

        passed, failed, skipped = 0, 0, 0
        for target, tests in self.tests_per_target.items():
            # Convert bools to int
            for test in tests:
                passed += int(test.passed())
                failed += int(test.failed())
                skipped += int(test.skipped())

            if failed > 0:
                self.runners[target].set_status(Runner.FAIL)
            else:
                self.runners[target].set_status(Runner.SUCCESS)

        return passed, failed, skipped
