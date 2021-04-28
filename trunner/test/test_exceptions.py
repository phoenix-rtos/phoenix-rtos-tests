import re

import pytest

from trunner.config import ALL_TARGETS
from trunner.testcase import TestCase


# Pytest tries to collect TestCase as a class to test
# Mark TestCase as not testable
TestCase.__test__ = False


class TestExceptionHandler:
    @staticmethod
    def assert_exc_msg(testcase, function_names):
        for func in function_names:
            pattern = rf'File "{__file__}", line \d+, in {func}'
            assert re.search(pattern, testcase.exception), \
                   f"pattern '{pattern}' not found in exception msg"

    @staticmethod
    def fake_harness(function):
        def harness(p):
            function()

        return harness

    @pytest.fixture
    def testcase(self):
        return TestCase(
            name='xyz',
            target=ALL_TARGETS[0],
            timeout=3,
            exec_bin='xyz'
        )

    def test_assert(self, testcase):
        def foo():
            assert False

        testcase.harness = TestExceptionHandler.fake_harness(foo)
        testcase.handle(proc=None, psh=False)

        TestExceptionHandler.assert_exc_msg(testcase, ('harness', 'foo'))

    def test_assert_msg(self, testcase):
        def foo():
            assert False, "boo has failed!"

        testcase.harness = TestExceptionHandler.fake_harness(foo)
        testcase.handle(proc=None, psh=False)

        TestExceptionHandler.assert_exc_msg(testcase, ('harness', 'foo'))
        assert "boo has failed!" in testcase.exception

    def test_exception(self, testcase):
        def foo():
            raise Exception("boo has failed!")

        testcase.harness = TestExceptionHandler.fake_harness(foo)
        testcase.handle(proc=None, psh=False)

        TestExceptionHandler.assert_exc_msg(testcase, ('harness', 'foo'))
        assert 'Exception: boo has failed!' in testcase.exception
