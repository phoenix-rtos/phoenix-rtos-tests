#
# Phoenix-RTOS test runner
#
# Tests for Mbedtls Test Suite harness
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau
#

import os
import threading

import pytest
import pexpect.fdpexpect

from trunner.harnesses.mbedtls import MbedtlsTestHarness
from trunner.harnesses.common import TestResult

TestResult.__test__ = False


def assert_result(result, expected):
    for res, expect in zip(result, expected):
        assert res.status == expect.status
        assert res.name == expect.name
        assert res.msg == expect.msg


def write_output(output, fifo, lock):
    with open(fifo, 'w') as f:
        f.write(output)
        f.flush()

        # We've written what we wanted to
        # Keep open fifo to not trigger EOF, hanging on a lock
        lock.acquire()


class TestMbedtlsHarness:
    """ Tests assert that Mbedtls harness parses Mbedtls Test Suite output properly """

    FIFO = "./test_harness.fifo"

    @pytest.fixture(scope="class")
    def fifo(self):
        os.mkfifo(self.FIFO)
        yield
        os.unlink(self.FIFO)

    def run_harness(self, writer_thr):
        lock = threading.Lock()
        lock.acquire()

        thread = threading.Thread(target=writer_thr, args=(lock,))
        thread.start()
        fifo_r = open(self.FIFO, 'r')

        proc = pexpect.fdpexpect.fdspawn(fifo_r, encoding='ascii', timeout=3)

        try:
            result = MbedtlsTestHarness.harness(proc)
        finally:
            lock.release()
            thread.join()
            fifo_r.close()

        return result

    def test_pass(self, fifo):
        expected = [
            TestResult(
                'Hash: RIPEMD160',
                TestResult.PASS,
            ),
            TestResult(
                'AES-192-ECB Encrypt NIST KAT #6',
                TestResult.PASS,
            ),
            TestResult(
                'XTEA Decrypt_ecb #1',
                TestResult.PASS,
            ),
            TestResult(
                'Check for MBEDTLS_AES_C when already present',
                TestResult.PASS,
            ),
            TestResult(
                'Diffie-Hellman load parameters from file [#1]',
                TestResult.PASS,
            ),
        ]

        def writer(lock):
            output = (f"{expected[0].name} ................................................... PASS\r\n"
                      f"{expected[1].name} ................................... PASS\r\n"
                      f"{expected[2].name} ............................................... PASS\r\n"
                      f"{expected[3].name} ...................... PASS\r\n"
                      f"{expected[4].name} ..................... PASS\r\n"
                      "PASSED (5 / 5 tests (0 skipped))\r\n")

            write_output(output, self.FIFO, lock)

        result = self.run_harness(writer)

        assert_result(result, expected)

    def test_fail(self, fifo):
        expected = [
            TestResult(
                'net_poll beyond FD_SETSIZE',
                TestResult.FAIL,
                msg=('\t\tdup2( got_fd, wanted_fd ) >= 0\n'
                     '\t\tat line 39, suites/test_suite_net.function\n')
            ),
            TestResult(
                'Overwrite 0 -> 3',
                TestResult.FAIL,
                msg=('\t\t( psa_its_set_wrap( uid, data2->len, data2->x, flags2 ) ) == PSA_SUCCESS\n'
                     '\t\tat line 129, suites/test_suite_psa_its.function\n'
                     '\t\tlhs = 0xffffffffffffff6e = -146\n'
                     '\t\trhs = 0x0000000000000000 = 0\n')
            ),
            TestResult(
                'Overwrite 3 -> 0',
                TestResult.FAIL,
                msg=('\t\t( psa_its_set_wrap( uid, data1->len, data1->x, flags1 ) ) == PSA_SUCCESS\n'
                     '\t\tat line 122, suites/test_suite_psa_its.function\n'
                     '\t\tlhs = 0xffffffffffffff6e = -146\n'
                     '\t\trhs = 0x0000000000000000 = 0\n')
            ),
        ]

        def writer(lock):
            output = (f"{expected[0].name} ........................................ FAILED\r\n"
                      "  dup2( got_fd, wanted_fd ) >= 0\r\n"
                      "  at line 39, suites/test_suite_net.function\r\n"
                      f"{expected[1].name} .................................................. FAILED\r\n"
                      "  ( psa_its_set_wrap( uid, data2->len, data2->x, flags2 ) ) == PSA_SUCCESS\r\n"
                      "  at line 129, suites/test_suite_psa_its.function\r\n"
                      "  lhs = 0xffffffffffffff6e = -146\r\n"
                      "  rhs = 0x0000000000000000 = 0\r\n"
                      f"{expected[2].name} .................................................. FAILED\r\n"
                      "  ( psa_its_set_wrap( uid, data1->len, data1->x, flags1 ) ) == PSA_SUCCESS\r\n"
                      "  at line 122, suites/test_suite_psa_its.function\r\n"
                      "  lhs = 0xffffffffffffff6e = -146\r\n"
                      "  rhs = 0x0000000000000000 = 0\r\n"
                      "----------------------------------------------------------------------------\r\n"
                      "FAILED (0 / 3 tests (0 skipped))")

            write_output(output, self.FIFO, lock)

        result = self.run_harness(writer)

        assert_result(result, expected)

    def test_skip(self, fifo):

        expected = [
            TestResult(
                'NIST KW self test',
                TestResult.IGNORE,
            ),
            TestResult(
                'NIST KW lengths #20 KW ciphertext NULL',
                TestResult.IGNORE,
            ),
            TestResult(
                'NIST KW wrap AES-256 CAVS 17.4 PLAINTEXT LENGTH = 4096 count 0',
                TestResult.IGNORE,
            ),
            TestResult(
                'NIST KWP unwrap AES-128 CAVS 21.4 PLAINTEXT LENGTH = 4096 count 0',
                TestResult.IGNORE,
            ),
            TestResult(
                'KW AES-128 wrap rfc 3394',
                TestResult.IGNORE,
            ),
        ]

        def writer(lock):
            output = f"{expected[0].name} ................................................. ----\r\n" \
                     + f"{expected[1].name} ............................ ----\r\n" \
                     + f"{expected[2].name} .... ----\r\n" \
                     + f"{expected[3].name} . ----\r\n" \
                     + f"{expected[4].name} .......................................... ----\r\n" \
                     + "----------------------------------------------------------------------------\r\n" \
                     + "PASSED (5 / 5 tests (5 skipped))"

            write_output(output, self.FIFO, lock)

        result = self.run_harness(writer)

        assert_result(result, expected)

    def test_mix(self, fifo):
        expected = [
            TestResult(
                'XTEA Decrypt_ecb #1',
                TestResult.PASS,
            ),
            TestResult(
                'Overwrite 3 -> 0',
                TestResult.FAIL,
                msg="\t\t( psa_its_set_wrap( uid, data1->len, data1->x, flags1 ) ) == PSA_SUCCESS\n"
                    "\t\tat line 122, suites/test_suite_psa_its.function\n"
                    "\t\tlhs = 0xffffffffffffff6e = -146\n"
                    "\t\trhs = 0x0000000000000000 = 0\n"
            ),
            TestResult(
                'NIST KWP unwrap AES-128 CAVS 21.4 PLAINTEXT LENGTH = 4096 count 0',
                TestResult.IGNORE,
            ),
        ]

        def writer(lock):
            output = (f"{expected[0].name} ............................................... PASS\r\n"
                      f"{expected[1].name} .................................................. FAILED\r\n"
                      "  ( psa_its_set_wrap( uid, data1->len, data1->x, flags1 ) ) == PSA_SUCCESS\r\n"
                      "  at line 122, suites/test_suite_psa_its.function\r\n"
                      "  lhs = 0xffffffffffffff6e = -146\r\n"
                      "  rhs = 0x0000000000000000 = 0\r\n"
                      f"{expected[2].name} . ----\r\n"
                      "----------------------------------------------------------------------------\r\n"
                      "FAILED (2 / 3 tests (1 skipped))")

            write_output(output, self.FIFO, lock)

        result = self.run_harness(writer)

        assert_result(result, expected)

    def test_wrong(self, fifo):
        def writer(lock):
            output = "Wrong mbedtls test suite output"

            write_output(output, self.FIFO, lock)

        with pytest.raises(pexpect.TIMEOUT):
            self.run_harness(writer)
