import copy
import shlex
from dataclasses import dataclass, field
from itertools import chain
from pathlib import Path

import yaml

from .tools.text import remove_prefix
from typing import Dict, List, Tuple


def resolve_phrtos_dir() -> Path:
    path = Path.cwd().absolute()
    try:
        # Find last occurrence of phoenix-rtos-project
        inverted_idx = path.parts[::-1].index('phoenix-rtos-project')
        idx = len(path.parts) - 1 - inverted_idx
    except ValueError:
        print('Runner is lanuched not from the phoenix-rtos-project directory')
        return path

    return Path(*path.parts[:idx+1])


PHRTOS_PROJECT_DIR = resolve_phrtos_dir()
PHRTOS_TEST_DIR = PHRTOS_PROJECT_DIR / 'phoenix-rtos-tests'

# Default time after pexpect will raise TIEMOUT exception if nothing matches an expected pattern
PYEXPECT_TIMEOUT = 8

# Available targets for test runner.
ALL_TARGETS = ['ia32-generic', 'host-pc', 'armv7m7-imxrt106x']

# Default targets used by parser if 'target' value is absent
DEFAULT_TARGETS = [target for target in ALL_TARGETS
                   if target not in ('host-pc')]

DEVICE_TARGETS = ['armv7m7-imxrt106x']

# Port to communicate with hardware board
DEVICE_SERIAL = "/dev/serial/by-path/platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.4:1.1"

# DEVICE_SERIAL USB port address
DEVICE_SERIAL_USB = "1-1.4"


def rootfs(target: str) -> Path:
    return PHRTOS_PROJECT_DIR / '_fs' / target / 'root'


class ParserError(Exception):
    pass


def array_value(array: Dict[str, List[str]]) -> List[str]:
    value = set(array.get('value', []))
    value |= set(array.get('include', []))
    value -= set(array.get('exclude', []))

    return list(value)


@dataclass
class ParserArgs:
    targets: List[str]
    yaml_path: Path
    path: Path = field(init=False)

    def __post_init__(self):
        self.path = self.yaml_path.parent


class Config(dict):
    def __init__(self, config: dict) -> None:
        self.update(**config)

    @classmethod
    def from_dict(cls, config: dict) -> 'Config':
        parser = ConfigParser()
        instance = cls(config)
        parser.parse(instance)
        return instance

    def join_targets(self, config: 'Config') -> None:
        if 'targets' not in config:
            return
        elif 'targets' not in self:
            self['targets'] = config['targets']
            return

        if 'value' in self['targets']:
            # The value field is defined. Do not overwrite it, just return.
            return

        targets = dict(config['targets'])
        targets.setdefault('value', ALL_TARGETS)
        value = array_value(targets)
        self['targets']['value'] = value

    def join(self, config: 'Config') -> None:
        self.join_targets(config)
        for key in config:
            if key not in self:
                self[key] = config[key]

    def setdefault_targets(self) -> None:
        targets = self.get('targets', dict())
        targets.setdefault('value', DEFAULT_TARGETS)
        self['targets'] = targets

    def setdefaults(self) -> None:
        self.setdefault('ignore', False)
        self.setdefault('type', 'unit')
        self.setdefault('timeout', PYEXPECT_TIMEOUT)
        self.setdefault_targets()


class TestConfig(Config):
    def __init__(self, config: dict) -> None:
        super().__init__(config)

    @classmethod
    def from_dict(cls, config: dict, main: Config, args: ParserArgs) -> 'TestConfig':
        parser = ConfigParser()
        instance = cls(config)
        parser.parse(instance)
        instance.join(main)
        instance.setdefaults()
        parser.resolve(instance, args)
        return instance

    def copy_per_target(self) -> List['TestConfig']:
        tests = []
        for target in array_value(self['targets']):
            test = copy.deepcopy(self)
            del test['targets']
            test['target'] = target
            tests.append(test)

        return tests


class ConfigParser:
    KEYWORDS: Tuple[str, ...] = ('exec', 'harness', 'ignore', 'name', 'targets', 'timeout', 'type')
    TEST_TYPES: Tuple[str, ...] = ('unit', 'harness')

    def parse_keywords(self, config: Config) -> None:
        keywords = set(config)
        unknown = keywords - set(self.KEYWORDS)
        if unknown:
            raise ParserError(f'Uknown keys: {", ".join(map(str, unknown))}')

    def parse_type(self, config: Config) -> None:
        test_type = config.get('type')
        if not test_type:
            return

        if test_type not in self.TEST_TYPES:
            msg = f'wrong test type: {test_type}. Allowed types: {", ".join(self.TEST_TYPES)}'
            raise ParserError(msg)

    def parse_harness(self, config: Config) -> None:
        harness = config.get('harness')
        if not harness:
            return

        harness = Path(harness)
        if not harness.suffix == '.py':
            raise ParserError(f'harness {harness} must be python script (with .py extension)')

        config['type'] = 'harness'
        config['harness'] = harness

    def parse_ignore(self, config: Config) -> None:
        ignore = config.get('ignore', False)

        if not isinstance(ignore, bool):
            raise ParserError(f'ignore must be a boolean value (true/false) not {ignore}')

    def parse_timeout(self, config: Config) -> None:
        timeout = config.get('timeout')
        if not timeout:
            return

        if not isinstance(timeout, int):
            try:
                timeout = int(timeout)
            except ValueError:
                raise ParserError(f'wrong timeout: {timeout}. It must be an integer with base 10')

        config['timeout'] = timeout

    @staticmethod
    def is_array(array: dict) -> bool:
        array_keys = {'value', 'include', 'exclude'}
        is_array = bool(array.keys() & array_keys)
        unknown_keys = array.keys() - array_keys
        return is_array and len(unknown_keys) == 0

    def parse_targets(self, config: Config) -> None:
        targets = config.get('targets')
        if not targets:
            return

        if not isinstance(targets, dict) or not ConfigParser.is_array(targets):
            raise ParserError('"targets" should be a dict with "value", "include", "exclude" keys.')

        for value in targets.values():
            unknown = set(value) - set(ALL_TARGETS)
            if unknown:
                raise ParserError(f'targets {", ".join(map(str, unknown))} are unknown')

    def parse_exec(self, config: Config) -> None:
        exec_cmd = config.get('exec')
        if not exec_cmd:
            return

        config['exec'] = shlex.split(exec_cmd)

    def parse(self, config: Config) -> None:
        self.parse_keywords(config)
        self.parse_targets(config)
        self.parse_harness(config)
        self.parse_type(config)
        self.parse_timeout(config)
        self.parse_ignore(config)
        self.parse_exec(config)

    def resolve_harness(self, config: Config, path: Path) -> None:
        harness = config.get('harness')
        if not harness:
            raise ParserError("'harness' keyword not found")
        harness = path / config['harness']
        if not harness.exists():
            raise ParserError(f'harness {harness} file not found')

        config['harness'] = harness

    def resolve_name(self, config: Config, path: Path) -> None:
        name = config.get('name')
        if not name:
            raise ParserError('Cannot resolve the test name')

        # Get a path relative to phoenix-rtos-project
        relative_path = remove_prefix(str(path), str(PHRTOS_PROJECT_DIR) + '/')
        name = f'{relative_path}/{name}'
        name = name.replace('/', '.')
        config['name'] = name

    def resolve_targets(self, config: Config, allowed_targets: List[str]) -> None:
        targets = array_value(config['targets'])
        targets = list(set(targets) & set(allowed_targets))
        config['targets'] = {'value': targets}

    def resolve(self, config: Config, args: ParserArgs) -> None:
        if 'harness' in config:
            self.resolve_harness(config, args.path)
        self.resolve_name(config, args.path)
        self.resolve_targets(config, args.targets)


@dataclass
class TestCaseConfig:
    main: Config
    tests: List[TestConfig]

    @staticmethod
    def load_yaml(path: Path) -> dict:
        with open(path, 'r') as f_yaml:
            config = yaml.safe_load(f_yaml)

        return config

    @staticmethod
    def extract_components(config: dict) -> Tuple[dict, List[dict]]:
        try:
            main = config.pop('test')
            tests = main.pop('tests')
        except KeyError as exc:
            raise ParserError(f'keyword {exc} not found in the test config')

        return main, tests

    @classmethod
    def from_dict(cls, config: dict, args: ParserArgs) -> 'TestCaseConfig':
        try:
            main, tests = TestCaseConfig.extract_components(config)
            main = Config.from_dict(main)
            tests = [TestConfig.from_dict(test, main, args) for test in tests]
            tests = [test.copy_per_target() for test in tests]
            tests = list(chain.from_iterable(tests))
        except ParserError as exc:
            raise ParserError(f'{args.path}: {exc}') from exc

        return cls(main, tests)

    @classmethod
    def from_yaml(cls, args: ParserArgs) -> 'TestCaseConfig':
        config = TestCaseConfig.load_yaml(args.yaml_path)
        return cls.from_dict(config, args)
