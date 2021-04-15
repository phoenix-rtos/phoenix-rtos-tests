import logging
import os
import pathlib


import yaml


PHRTOS_PROJECT_DIR = pathlib.Path(os.getcwd())
PHRTOS_TEST_DIR = PHRTOS_PROJECT_DIR / 'phoenix-rtos-tests'

# Default time after pexpect will raise TIEMOUT exception if nothing matches an expected pattern
PYEXPECT_TIMEOUT = 8

def remove_prefix(string, prefix):
    if string.startswith(prefix):
        return string[len(prefix):]

    return string


class YAMLParserError(Exception):
    """Exception raised when config has wrong format"""

    def __init__(self, parser, message="Parser error"):
        message = f"{parser.path}:\n{message}"
        super().__init__(message)


class YAMLParser:
    """This class is responsible for parsing YAML config that defines test cases"""

    def __init__(self, path):
        # Current path is /example/test/test.yaml - get rid of test.yaml in path
        self.path = path.parents[0]

        with open(path, 'r') as f_yaml:
            try:
                self.config = yaml.safe_load(f_yaml)
            except yaml.YAMLError as exc:
                raise YAMLParserError(self, exc) from exc

        self.parsed_tests = []

    def parse_test_case(self, config):
        allowed_keys = ('exec', 'type', 'harness', 'name', 'ignore', 'timeout')
        mandatory_keys = ('exec', 'name')

        unnecessary_keys = set(config) - set(allowed_keys)
        if unnecessary_keys:
            logging.debug(f"Dropping unnecessary keys: {unnecessary_keys}\n")
            for key in unnecessary_keys:
                config.pop(key)

        for key in mandatory_keys:
            if key not in config:
                raise YAMLParserError(self, f"key {key} not found in test config")

        # Extend test name with path
        test_path = remove_prefix(str(self.path), str(PHRTOS_PROJECT_DIR) + '/')
        name = f"{test_path}/{config['name']}"
        config['name'] = name.replace('/', '.')

        # If type is not known assume that it's a unit test
        config.setdefault('type', 'unit')
        config.setdefault('ignore', False)
        config.setdefault('timeout', PYEXPECT_TIMEOUT)

        if not isinstance(config['ignore'], bool):
            config['ignore'] = False

        if config.get('harness'):
            config['type'] = 'harness'
            config['harness'] = self.path / config['harness']

            if not config['harness'].exists():
                raise YAMLParserError(self, f"harness {config['harness']} file not found")

            if not str(config['harness']).endswith('.py'):
                raise YAMLParserError(self, "harness file must has .py extension")

        return config

    def parse_test_config(self):
        config = self.config
        if 'test' not in config:
            raise YAMLParserError(self, 'Keyword "test" not found in test config')

        config = config['test']
        if 'tests' not in config:
            raise YAMLParserError(self, 'Keyword "tests" not found in test config')

        for test_config in config['tests']:
            # Combine 'local' and 'global' config
            for key in config:
                if key not in test_config and key != 'tests':
                    test_config[key] = config[key]

            test = self.parse_test_case(test_config)
            self.parsed_tests.append(test)

        return self.parsed_tests
