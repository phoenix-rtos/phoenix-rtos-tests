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

from trunner.tools.color import Color


def prepare_info(testcases_info=[]):
    print('prepare')
    for i, info in enumerate(testcases_info):
        print(i)
        testcases_info[i] = testcases_info[i].replace('  ', '\t\t')
        testcases_info[i] = testcases_info[i].replace('\r', '')

    return testcases_info


def assert_result(result, state=TestResult.PASS, names=[], fails_info=[]):

    color = {
        TestResult.PASS: Color.OK,
        TestResult.IGNORE: Color.SKIP,
        TestResult.FAIL: Color.FAIL}

    status = Color.colorify(state, color[state])

    for i in range(len(names)):
        testcase_result = f'{result[i]}'
        expected = f'{status}: {names[i]}'
        if state == TestResult.FAIL:
            expected += '\n\t\tOutput of the failed test case:\n\t\t---\n'
            expected += f'{fails_info[i]}'
            expected += '\t\t---'

        assert testcase_result == expected


def write_output(output, file, lock):
    with open(file, 'w') as f:
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

    def get_result(self, writer_thr):
        lock = threading.Lock()
        lock.acquire()

        thread = threading.Thread(target=writer_thr, args=(lock,))
        thread.start()
        fifo_r = open(self.FIFO, 'r')

        proc = pexpect.fdpexpect.fdspawn(fifo_r, encoding='ascii', timeout=5)
        result = MbedtlsTestHarness.harness(proc)

        # Release lock for writer to close fifo
        lock.release()
        thread.join()
        fifo_r.close()

        return result

    def test_pass(self, fifo):
        names = ['Hash: RIPEMD160',
                 'AES-192-ECB Encrypt NIST KAT #6',
                 'XTEA Decrypt_ecb #1',
                 'Check for MBEDTLS_AES_C when already present',
                 'Diffie-Hellman load parameters from file [#1]']

        def writer(lock):
            output = f'{names[0]} ................................................... PASS\r\n' \
                   + f'{names[1]} ................................... PASS\r\n' \
                   + f'{names[2]} ............................................... PASS\r\n' \
                   + f'{names[3]} ...................... PASS\r\n' \
                   + f'{names[4]} ..................... PASS\r\n' \
                   + 'PASSED (5 / 5 tests (0 skipped))\r\n'

            write_output(output, self.FIFO, lock)

        result = self.get_result(writer)
        assert_result(result, TestResult.PASS, names)

    def test_fail(self, fifo):
        names = ['net_poll beyond FD_SETSIZE',
                 'Overwrite 0 -> 3',
                 'Overwrite 3 -> 0']

        testcases_info = ['  dup2( got_fd, wanted_fd ) >= 0\r\n'
                          + '  at line 39, suites/test_suite_net.function\r\n',
                          '  ( psa_its_set_wrap( uid, data2->len, data2->x, flags2 ) ) == PSA_SUCCESS\r\n'
                          + '  at line 129, suites/test_suite_psa_its.function\r\n'
                          + '  lhs = 0xffffffffffffff6e = -146\r\n'
                          + '  rhs = 0x0000000000000000 = 0\r\n',
                          '  ( psa_its_set_wrap( uid, data1->len, data1->x, flags1 ) ) == PSA_SUCCESS\r\n'
                          + '  at line 122, suites/test_suite_psa_its.function\r\n'
                          + '  lhs = 0xffffffffffffff6e = -146\r\n'
                          + '  rhs = 0x0000000000000000 = 0\r\n']

        def writer(lock):
            output = f'{names[0]} ........................................ FAILED\r\n' \
                     + f'{testcases_info[0]}' \
                     + f'{names[1]} .................................................. FAILED\r\n' \
                     + f'{testcases_info[1]}' \
                     + f'{names[2]} .................................................. FAILED\r\n' \
                     + f'{testcases_info[2]}' \
                     + '----------------------------------------------------------------------------\r\n' \
                     + 'FAILED (0 / 3 tests (0 skipped))'

            with open(self.FIFO, 'w') as f:
                f.write(output)
                f.flush()

                # We've written what we wanted to
                # Keep open fifo to not trigger EOF, hanging on a lock
                lock.acquire()

        result = self.get_result(writer)
        testcases_info = prepare_info(testcases_info)

        assert_result(result, TestResult.FAIL, names, testcases_info)

    def test_skip(self, fifo):
        names = ['NIST KW self test',
                 'NIST KW lengths #20 KW ciphertext NULL',
                 'NIST KW wrap AES-256 CAVS 17.4 PLAINTEXT LENGTH = 4096 count 0',
                 'NIST KWP unwrap AES-128 CAVS 21.4 PLAINTEXT LENGTH = 4096 count 0',
                 'KW AES-128 wrap rfc 3394']

        def writer(lock):
            output = f'{names[0]} ................................................. ----\r\n' \
                     + f'{names[1]} ............................ ----\r\n' \
                     + f'{names[2]} .... ----\r\n' \
                     + f'{names[3]} . ----\r\n' \
                     + f'{names[4]} .......................................... ----\r\n' \
                     + '----------------------------------------------------------------------------\r\n' \
                     + 'PASSED (5 / 5 tests (4 skipped))'

            write_output(output, self.FIFO, lock)

        result = self.get_result(writer)
        assert_result(result, TestResult.IGNORE, names)

    def test_mix(self, fifo):
        names = ['XTEA Decrypt_ecb #1',
                 'Overwrite 3 -> 0',
                 'NIST KWP unwrap AES-128 CAVS 21.4 PLAINTEXT LENGTH = 4096 count 0']

        testcase_info = '  ( psa_its_set_wrap( uid, data1->len, data1->x, flags1 ) ) == PSA_SUCCESS\r\n' \
                        + '  at line 122, suites/test_suite_psa_its.function\r\n' \
                        + '  lhs = 0xffffffffffffff6e = -146\r\n' \
                        + '  rhs = 0x0000000000000000 = 0\r\n'

        def writer(lock):
            output = f'{names[0]} ............................................... PASS\r\n' \
                     + f'{names[1]} .................................................. FAILED\r\n' \
                     + f'{testcase_info}' \
                     + f'{names[2]} . ----\r\n' \
                     + '----------------------------------------------------------------------------\r\n' \
                     + 'FAILED (2 / 3 tests (1 skipped))'

            write_output(output, self.FIFO, lock)

        result = self.get_result(writer)

        temp = [testcase_info, ]
        temp = prepare_info(temp)
        testcase_info = temp[0]

        assert_result([result[0], ], TestResult.PASS, [names[0], ])
        assert_result([result[1], ], TestResult.FAIL, [names[1], ], [testcase_info, ])
        assert_result([result[2], ], TestResult.IGNORE, [names[2], ])
