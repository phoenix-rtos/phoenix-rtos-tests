import logging
import os
import pathlib

import yaml

from .tools.text import remove_prefix

PHRTOS_PROJECT_DIR = pathlib.Path(os.getcwd())
PHRTOS_TEST_DIR = PHRTOS_PROJECT_DIR / 'phoenix-rtos-tests'

# Default time after pexpect will raise TIEMOUT exception if nothing matches an expected pattern
PYEXPECT_TIMEOUT = 8

# Available targets for test runner.
ALL_TARGETS = ['ia32-generic', 'host-pc', 'armv7m7-imxrt106x']

# Default targets used by parser if 'target' value is absent
DEFAULT_TARGETS = [target for target in ALL_TARGETS
                   if target not in ('host-pc', 'armv7m7-imxrt106x')]

DEVICE_TARGETS = ['armv7m7-imxrt106x']

# Port to communicate with hardware board
DEVICE_SERIAL = "/dev/ttyACM0"


class YAMLParserError(Exception):
    """Exception raised when config has wrong format"""

    def __init__(self, parser, message="Parser error"):
        message = f"{parser.path}:\n{message}"
        super().__init__(message)


class YAMLParser:
    """This class is responsible for parsing YAML config that defines test cases"""

    def __init__(self, path=None, targets=None):
        self.config = None
        if targets is None:
            targets = ALL_TARGETS
        self.targets = targets
        self.parsed_tests = []

        if path is None:
            self.path = ''
            return

        self.path = path
        self.load_yaml(self.path)
        # Current path is /example/test/test.yaml - get rid of test.yaml in path
        self.path = self.path.parents[0]

    def load_yaml(self, path):
        with open(path, 'r') as f_yaml:
            try:
                self.config = yaml.safe_load(f_yaml)
            except yaml.YAMLError as exc:
                raise YAMLParserError(self, exc) from exc

    def parse_keywords(self, test):
        allowed_keys = ('exec', 'type', 'harness', 'name', 'ignore', 'timeout', 'targets')
        mandatory_keys = ('exec', 'name')

        unnecessary_keys = set(test) - set(allowed_keys)
        if unnecessary_keys:
            logging.debug(f"Dropping unnecessary keys: {unnecessary_keys}\n")
            for key in unnecessary_keys:
                test.pop(key)

        for key in mandatory_keys:
            if key not in test:
                raise YAMLParserError(self, f"key {key} not found in test config")

    def parse_name(self, test):
        # Extend test name with path
        test_path = remove_prefix(str(self.path), str(PHRTOS_PROJECT_DIR) + '/')
        name = f"{test_path}/{test['name']}"
        test['name'] = name.replace('/', '.')

    def parse_harness(self, test):
        test['type'] = 'harness'
        test['harness'] = self.path / test['harness']

        if not test['harness'].exists():
            raise YAMLParserError(self, f"harness {test['harness']} file not found")

        if not str(test['harness']).endswith('.py'):
            raise YAMLParserError(self, "harness file must has .py extension")

    @staticmethod
    def parse_array_value(array):
        value = set(array.get('value', []))
        value |= set(array.get('include', []))
        value -= set(array.get('exclude', []))

        return value

    def parse_target(self, test):
        target_array = test.get('targets', dict())
        target_array.setdefault('value', DEFAULT_TARGETS)

        targets = YAMLParser.parse_array_value(target_array)
        unknown_targets = targets - set(ALL_TARGETS)
        if unknown_targets:
            raise YAMLParserError(self, f"wrong target: {', '.join(map(str, unknown_targets))}")

        targets &= set(self.targets)
        test['targets'] = {'value': list(targets)}

    def parse_test_case(self, test):
        self.parse_keywords(test)
        self.parse_name(test)
        self.parse_target(test)
        # If type is not known assume that it's a unit test
        test.setdefault('type', 'unit')
        test.setdefault('timeout', PYEXPECT_TIMEOUT)

        test.setdefault('ignore', False)
        if not isinstance(test['ignore'], bool):
            test['ignore'] = False

        if test.get('harness'):
            self.parse_harness(test)

        # Copy test per target
        test_case = []
        for target in test['targets']['value']:
            test_by_target = test.copy()
            test_by_target['target'] = target
            test_case.append(test_by_target)
            del test_by_target['targets']

        return test_case

    def inherit_array_value(self, test, keyword):
        global_array = self.config.get(keyword)
        array = test.get(keyword)

        if global_array and array:
            array.setdefault('value', global_array['value'])

    def inherit_global_keywords(self, test):
        self.inherit_array_value(test, 'targets')

        for key in self.config:
            if key not in test:
                test[key] = self.config[key]

    def parse_test_config(self):
        if 'test' not in self.config:
            raise YAMLParserError(self, 'Keyword "test" not found in test config')

        self.config = self.config['test']
        if 'tests' not in self.config:
            raise YAMLParserError(self, 'Keyword "tests" not found in test config')

        if 'targets' in self.config:
            self.parse_target(self.config)

        tests_configs = self.config.pop('tests')
        for test_config in tests_configs:
            # Combine 'local' test config and 'global' config
            self.inherit_global_keywords(test_config)
            test_case = self.parse_test_case(test_config)
            self.parsed_tests.extend(test_case)

        return self.parsed_tests
