import os
import re
import threading

import pytest
import pexpect.fdpexpect

from trunner.config import ALL_TARGETS
from trunner.testcase import TestCase


# Pytest tries to collect TestCase as a class to test
# Mark TestCase as not testable
TestCase.__test__ = False


class TestPexpectException:
    """ Tests check if PyExpect exceptions are handled properly """

    FIFO = "./test_exceptions.fifo"

    @pytest.fixture
    def testcase(self):
        return TestCase(
            name='xyz',
            target=ALL_TARGETS[0],
            timeout=3,
            exec_cmd=['xyz'],
            psh=False
        )

    @pytest.fixture(scope="class")
    def fifo(self):
        os.mkfifo(self.FIFO)
        yield
        os.unlink(self.FIFO)

    @staticmethod
    def fake_harness(proc):
        proc.expect_exact("Hello ")
        proc.expect_exact("world!")

    def test_timeout(self, fifo, testcase):
        def writer(lock):
            with open(self.FIFO, 'w') as f:
                f.write("Hello universe!")
                f.flush()

                # We've written what we wanted to
                # Keep open fifo to not trigger EOF, hanging on a lock
                lock.acquire()

        lock = threading.Lock()
        lock.acquire()

        thread = threading.Thread(target=writer, args=(lock,))
        thread.start()

        fifo_r = open(self.FIFO, 'r')

        proc = pexpect.fdpexpect.fdspawn(fifo_r, encoding='ascii', timeout=1)
        testcase.harness = TestPexpectException.fake_harness
        testcase.handle(proc)

        # Release lock for writer to close fifo
        lock.release()
        thread.join()
        fifo_r.close()

        assert testcase.failed()
        assert "EXCEPTION TIMEOUT" in testcase.exception

        # We should have read word "Hello "
        # Check if read/expect buffers content are mentioned in the exception
        assert "Hello " not in testcase.exception
        assert "world!" in testcase.exception
        assert "universe!" in testcase.exception

    def test_eof(self, fifo, testcase):
        def writer():
            with open(self.FIFO, 'w') as f:
                f.write("Hello universe!")

        thread = threading.Thread(target=writer)
        thread.start()

        fifo_r = open(self.FIFO, 'r')

        # Wait for end of write to get EOF
        thread.join()

        proc = pexpect.fdpexpect.fdspawn(fifo_r, encoding='ascii')
        testcase.harness = TestPexpectException.fake_harness
        testcase.handle(proc)

        fifo_r.close()

        assert testcase.failed()
        assert "EXCEPTION EOF" in testcase.exception

        # We should have read word "Hello "
        # Check if read/expect buffers content are mentioned in the exception
        assert "Hello " not in testcase.exception
        assert "world!" in testcase.exception
        assert "universe!" in testcase.exception


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
            exec_cmd=['xyz'],
            psh=False
        )

    def test_assert(self, testcase):
        def foo():
            assert False

        testcase.harness = TestExceptionHandler.fake_harness(foo)
        testcase.handle(proc=None)

        TestExceptionHandler.assert_exc_msg(testcase, ('harness', 'foo'))

    def test_assert_msg(self, testcase):
        def foo():
            assert False, "boo has failed!"

        testcase.harness = TestExceptionHandler.fake_harness(foo)
        testcase.handle(proc=None)

        TestExceptionHandler.assert_exc_msg(testcase, ('harness', 'foo'))
        assert "boo has failed!" in testcase.exception

    def test_exception(self, testcase):
        def foo():
            raise Exception("boo has failed!")

        testcase.harness = TestExceptionHandler.fake_harness(foo)
        testcase.handle(proc=None)

        TestExceptionHandler.assert_exc_msg(testcase, ('harness', 'foo'))
        assert 'Exception: boo has failed!' in testcase.exception
