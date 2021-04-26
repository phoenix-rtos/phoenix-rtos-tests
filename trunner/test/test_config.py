import pathlib

import pytest

from trunner.config import YAMLParser, YAMLParserError, ALL_TARGETS, DEFAULT_TARGETS


@pytest.fixture
def parser(config, targets):
    path = pathlib.Path(f'trunner/test/yamls/{config}')
    return YAMLParser(path, targets)


class TestTarget:
    @pytest.fixture
    def targets(self):
        return ALL_TARGETS

    @pytest.mark.parametrize('config, expect_target', [
        (
            'target-test-0.yaml',
            [
                ['host-pc']
            ]
        ),
        (
            'target-test-1.yaml',
            [
                ['host-pc'],
                ['host-pc', 'ia32-generic'],
                ['ia32-generic']
            ]
        ),
        (
            'target-test-2.yaml',
            [
                ['ia32-generic'],
                ['host-pc'],
                ['host-pc']
            ]
        ),
    ])
    def test_target_keyword(self, parser, expect_target):
        tests = parser.parse_test_config()

        # Group test targets by name
        test_targets = {test['name']: list() for test in tests}
        for test in tests:
            test_targets[test['name']].append(test['target'])

        for idx, target in enumerate(expect_target):
            name = f'trunner.test.yamls.test_{idx}'
            assert sorted(test_targets[name]) == sorted(target)

    @pytest.mark.parametrize('config', [
        'empty-target-test-0.yaml'
    ])
    def test_empty_target(self, parser):
        tests = parser.parse_test_config()
        assert not tests

    @pytest.mark.parametrize('target, expect_target', [
        (
            {},
            DEFAULT_TARGETS
        ),
        (
            {'include': ['host-pc']},
            ALL_TARGETS
        ),
        (
            {'exclude': ['ia32-generic']},
            [target for target in DEFAULT_TARGETS if target != 'ia32-generic']
        ),
        (
            {'value': ['host-pc']},
            ['host-pc']
        ),
        (
            {'value': ['ia32-generic'],
             'exclude': ['ia32-generic']},
            []
        ),
        (
            {'value': ['ia32-generic'],
             'include': ['ia32-generic'],
             'exclude': ['ia32-generic']},
            []
        ),
    ])
    def test_parse_target(self, target, expect_target):
        # Create minimal test config that can be parsed
        test = {'targets': target}
        YAMLParser().parse_target(test)
        assert sorted(test['targets']['value']) == sorted(expect_target)

    @pytest.mark.parametrize('target', [
        {'value': ['turing-machine']},
        {'value': ['register-machine']},
    ])
    def test_parse_target_exc(self, target):
        # Create minimal test config that can be parsed
        test = {'targets': target}
        with pytest.raises(YAMLParserError):
            YAMLParser().parse_target(test)
