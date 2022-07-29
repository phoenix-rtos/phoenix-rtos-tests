#
# Phoenix-RTOS test runner
#
# Tests for MicroPython Test Suite harness
#
# Copyright 2022 Phoenix Systems
# Authors: Piotr Nieciecki
#

import os
import threading
import pytest
import re
import pexpect
import pexpect.fdpexpect
import socket

from trunner.harnesses.micropython import MicropythonStandardHarness
from trunner.harnesses.micropython import MicropythonReplHarness
from trunner.harnesses.micropython import create_regex
from trunner.harnesses.micropython import standard_regex_escape
from trunner.harnesses.common import TestResult

TestResult.__test__ = False

PROMPT = "\r\u001b[0J(psh)% "

MICROPYTHON = '/bin/micropython'
UPYTHON_HEADER = '''MicroPython 6d2ae58-dirty on 2022-07-27; linux version\r
Use Ctrl-D to exit, Ctrl-E for paste mode\r
>>> '''
UPYTHON_PROMPT = "\r\n>>>"
EXP_UPYTHON_HEADER = "MicroPython \\.\\+ version\nUse \\.\\+\n>>> "


def check_result(result, exp_result):
    result = result[0]

    assert result.name == exp_result.name
    assert result.status == exp_result.status

    if exp_result.msg is not None:
        assert result.msg == exp_result.msg


class MicropythonMock:

    SOCKET_FILE = "/tmp/test_micropython"

    def __init__(self):
        self.socket = None
        self.success = True
        self.error_msg = None

    def send(self, text):
        self.socket.send(text.encode("ascii"))

    def recv_exp(self, exp_output):
        res = self.socket.recv(len(exp_output))
        res = res.decode("ascii")

        assert res == exp_output, f"Error - callback didn't received \"{exp_output}\", but got \"{res}\""

    def get_output(self, harness, callback, *callback_args):
        """Function uses callback function to communicate with specified harness. Returns harness result"""

        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        if os.path.exists(self.SOCKET_FILE):
            os.remove(self.SOCKET_FILE)
        s.bind(self.SOCKET_FILE)

        lock = threading.Lock()
        lock.acquire()

        thread = threading.Thread(target=self.__start_callback, args=(lock, callback, *callback_args))
        thread.start()

        s.listen()
        lock.release()
        conn, _ = s.accept()
        s.close()

        proc = pexpect.fdpexpect.fdspawn(conn.fileno(), encoding="ascii")

        try:
            result = harness(proc)
        finally:
            conn.close()
            thread.join()
            os.remove(self.SOCKET_FILE)

        assert self.success is True, self.error_msg

        return result

    def __connect(self):
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.connect(self.SOCKET_FILE)

    def __close(self):
        self.socket.close()

    def __start_callback(self, lock, callback, *callback_args):
        lock.acquire()
        self.__connect()

        try:
            callback(self, *callback_args)
        except Exception as e:
            self.success = False
            self.error_msg = str(e)
        finally:
            self.__close()


class TestTools:

    def test_standard_regex_escape(self):

        def get_outputs(inputs):
            return [standard_regex_escape(input) for input in inputs]

        # Testing escaping special characters
        inputs = ['[', ']', '.', '^', '$', '*', '+', '?', '{', '}', '|', '(', ')']
        exp_outputs = ['\\[', '\\]', '\\.', '\\^', '\\$', '\\*', '\\+', '\\?', '\\{', '\\}', '\\|', '\\(', '\\)']

        assert exp_outputs == get_outputs(inputs)

        # Testing special characters
        inputs = ['\\[', '\\]', '\\\\', '\\.', '\\^', '\\$', '\\*', '\\+', '\\?', '\\{', '\\}', '\\|', '\\(', '\\)']
        exp_outputs = ['[', ']', '\\', '.', '^', '$', '*', '+', '?', '{', '}', '|', '(', ')']

        assert exp_outputs == get_outputs(inputs)

        # Testing text without special characters
        inputs = [
            'abcdefg',
            'zxcvbnm',
            '12323436',
            '09878765',
            '\t\v'
        ]
        exp_outputs = inputs

        assert exp_outputs == get_outputs(inputs)

        # Testing text with special characters
        inputs = [
            'abc\\[1-2\\]xyz',
            'GHJKL\\??{\\}\\}345',
            'qwer\\.poiuyy\\*\\'
        ]
        exp_outputs = [
            'abc[1-2]xyz',
            'GHJKL?\\?\\{}}345',
            'qwer.poiuyy*'
        ]

        assert exp_outputs == get_outputs(inputs)

    def test_create_regex(self):

        def create_and_check_regex(text_for_regex, matching_texts):
            regex = create_regex(text_for_regex)
            for text in matching_texts:
                assert re.fullmatch(regex, text) is not None

        # Testing checking regex for a number
        input = 'abc\\\\d\\+xyz'
        matching_texts = [
            'abc1xyz',
            'abc12xyz',
            'abc123xyz',
            'abc12345678912345678xyz',
            'abc9999999xyz'
        ]

        create_and_check_regex(input, matching_texts)

        # Testing new line handling
        input = 'abc\r\r\r\nxyz'
        matching_texts = [
            'abc\r\nxyz',
            'abc\r\r\nxyz',
            'abc\r\r\r\nxyz',
            'abc\r\r\r\r\nxyz'
        ]

        create_and_check_regex(input, matching_texts)

        # Testing handling ########
        # In MicroPython tests ######## matches zero or more lines with text
        input = '########'
        matching_texts = [
            '\n',
            'Elizabeth have a cat\n',
            'Hello world\nHello people\n',
            '''Lorem ipsum dolor sit amet,
             consectetur adipiscing elit,
             sed do eiusmod tempor incididunt ut
             labore et dolore magna aliqua.
             '''
        ]

        create_and_check_regex(input, matching_texts)

        # Testing sequence which can lead to incorrect regex
        # Some tests uses such a syntax
        input = '########\r\r\n\\.\\+00'
        matching_texts = [
            '00',
            'Hello world - 00',
            '''Lorem ipsum dolor sit amet,
            consectetur adipiscing elit,
            sed do eiusmod tempor incididunt ut
            labore et dolore magna aliqua. 00'''
        ]

        create_and_check_regex(input, matching_texts)

        # Testing new lines around ########
        input = 'start\r\n########\r\nstop'
        matching_texts = [
            '''start\r\nstop''',

            '''start\r\nline1\r\nstop''',

            '''start\r\nline1\r\nline2\r\nstop''',

            '''start\r\nline1\r\r\nline2\r\r\r\nstop'''
        ]

        create_and_check_regex(input, matching_texts)


class TestStandardHarness:

    # Program for communicating with harness
    def harness_callback(mock, *args):
        program_output = args[0]
        mock.send(program_output)

        if len(args) > 1:
            test_path = args[1]
            exp_output = args[2]
            mock.recv_exp(f"cat {test_path}.exp\n")
            mock.send(exp_output)

    def test_standard_pass(self):
        test_path = "/usr/test/micropython/basics/test.py"
        program_output = f"Running test: {test_path}\r\nLorem ipsum{PROMPT}"
        exp_output = f"cat {test_path}.exp\r\nLorem ipsum{PROMPT}"

        result = MicropythonMock().get_output(MicropythonStandardHarness().harness,
                                              TestStandardHarness.harness_callback,
                                              program_output, test_path, exp_output)

        check_result(result, TestResult(test_path, TestResult.PASS))

    def test_long_pass(self):
        test_path = "/usr/test/micropython/basics/test.py"

        text = '''Lorem ipsum dolor sit amet, consectetur adipiscing elit.
        Nulla elementum, mauris pretium consectetur vestibulum, lectus nibh gravida velit,
        ac interdum lectus ligula et quam.
        Vestibulum ac nunc vel felis sodales bibendum vitae sit amet magna.
        Curabitur elit mi, congue et dui vel, auctor pellentesque dolor.
        Vestibulum lobortis placerat justo, iaculis placerat nibh dictum placerat.
        Donec sapien leo, faucibus sit amet cursus eu, convallis eget erat.
        Curabitur tincidunt nibh vel eros accumsan, eu vestibulum mi ullamcorper.'''

        program_output = f'Running test: {test_path}\r\n{text}{PROMPT}'

        exp_output = f'cat {test_path}.exp\r\n{text}{PROMPT}'

        result = MicropythonMock().get_output(MicropythonStandardHarness().harness,
                                              TestStandardHarness.harness_callback,
                                              program_output, test_path, exp_output)

        check_result(result, TestResult(test_path, TestResult.PASS))

    def test_program_error(self):
        test_path = "/usr/test/micropython/basics/test.py"

        program_name = "/bin/test_harness"
        error_desc = "Some kind of error"
        error = f"Error: {program_name} - {error_desc}"

        program_output = f"Running test: {test_path}\r\n{error}"

        msg = f"There was an error {error_desc} during program {program_name} execution:\n{program_output}\n"
        with pytest.raises(AssertionError, match=msg):
            MicropythonMock().get_output(MicropythonStandardHarness().harness,
                                         TestStandardHarness.harness_callback,
                                         program_output + PROMPT)

    def test_no_path(self):
        """Testing what happens when no test path is returned"""

        test_path = ""
        program_output = f"Running test: {test_path}\r\nLorem ipsum"

        with pytest.raises(AssertionError, match=f"Can't find path to the test\nProgram output:\n{program_output}\n"):
            MicropythonMock().get_output(MicropythonStandardHarness().harness,
                                         TestStandardHarness.harness_callback,
                                         program_output + PROMPT)

    def test_skip(self):
        test_path = "/usr/test/micropython/basics/test.py"
        program_output = f"Running test: {test_path}\r\nSKIP{PROMPT}"

        result = MicropythonMock().get_output(MicropythonStandardHarness().harness,
                                              TestStandardHarness.harness_callback,
                                              program_output)

        exp_result = TestResult(test_path,
                                TestResult.IGNORE,
                                "Test uses modules, which are disabled in our implementation")

        check_result(result, exp_result)

    def test_incorrect_result(self):
        test_path = "/usr/test/micropython/basics/test.py"
        program_output = f"Running test: {test_path}\r\nSome result{PROMPT}"
        exp_output = f"cat {test_path}.exp\r\nDifferent result{PROMPT}"

        result = MicropythonMock().get_output(MicropythonStandardHarness().harness,
                                              TestStandardHarness.harness_callback,
                                              program_output, test_path, exp_output)

        exp_result = TestResult(test_path, TestResult.FAIL,
                                "Incorrect result!\n\nExpected result:\nDifferent result\nTest result:\nSome result\n")

        check_result(result, exp_result)

    def test_with_regex_output_pass(self):
        test_path = "test/special_test.py"

        # Adding this test path to test with regex output
        MicropythonStandardHarness.TESTS_WITH_REGEX_OUTPUT = {test_path, }

        test_path = f"/usr/test/micropython/{test_path}"

        text = "aaa\r\n123\r\ngray\r\ngrey"
        regex = "a\\{3\\}\r\n\\\\d\\+\r\ngr\\(a\\|e\\)y\r\ngr\\(a\\|e\\)y"

        program_output = f"Running test: {test_path}\r\n{text}{PROMPT}"
        exp_output = f"cat {test_path}.exp\r\n{regex}{PROMPT}"

        result = MicropythonMock().get_output(MicropythonStandardHarness().harness,
                                              TestStandardHarness.harness_callback,
                                              program_output, test_path, exp_output)

        check_result(result, TestResult(test_path, TestResult.PASS))

    def test_with_regex_output_fail(self):
        test_path = "test/special_test.py"

        # Adding this test path to test with regex output
        MicropythonStandardHarness.TESTS_WITH_REGEX_OUTPUT = {test_path, }

        test_path = f"/usr/test/micropython/{test_path}"

        text = "abc"
        regex = "\\\\d\\+"

        program_output = f"Running test: {test_path}\r\n{text}{PROMPT}"
        exp_output = f"cat {test_path}.exp\r\n{regex}{PROMPT}"

        result = MicropythonMock().get_output(MicropythonStandardHarness().harness,
                                              TestStandardHarness.harness_callback,
                                              program_output, test_path, exp_output)

        exp_result = TestResult(test_path, TestResult.FAIL,
                                f"Incorrect result!\n\nExpected result:\n{regex}\nTest result:\n{text}\n")
        check_result(result, exp_result)


class TestReplHarness:

    # Program for communicating with harness
    def harness_callback(mock, *args):
        if len(args) == 0:
            return

        test_path = args[0]

        # echo with test path
        mock.send(test_path + PROMPT)

        # receiving cat with path to test commands
        mock.recv_exp(f"cat {test_path}\n")

        if len(args) == 1:
            return

        program_commands = args[1]

        commands = "\r\n".join(program_commands)
        commands += PROMPT
        mock.send(commands)

        # receiving cat with path to expected output
        mock.recv_exp(f"cat {test_path}.exp\n")

        if len(args) == 2:
            return

        cmd_outputs = args[2]
        exp_output = args[3]

        mock.send(exp_output + PROMPT)

        # receiving command to open MicroPython
        mock.recv_exp(MICROPYTHON + '\n')
        mock.send(UPYTHON_HEADER)

        for cmd, out in zip(program_commands, cmd_outputs):
            mock.recv_exp(cmd + '\n')
            mock.send(cmd + '\r\n' + out + UPYTHON_PROMPT)

    def test_repl_pass(self):
        test_path = "/usr/test/micropython/test.py"

        program_cmd = ['2+2', ]
        cmd_outputs = [f'4{UPYTHON_PROMPT}', ]
        exp_output = EXP_UPYTHON_HEADER + program_cmd[0] + '\r\n' + cmd_outputs[0]

        result = MicropythonMock().get_output(MicropythonReplHarness().harness,
                                              TestReplHarness.harness_callback,
                                              test_path, program_cmd, cmd_outputs, exp_output)

        check_result(result, TestResult(test_path, TestResult.PASS))

    def test_repl_multiline_pass(self):
        test_path = "/usr/test/micropython/test.py"

        program_cmd = ['print("Lorem ipsum")', '2+2', '(1, 2, 3)']
        cmd_outputs = ['Lorem ipsum', '4', '(1, 2, 3)']
        for cmd in cmd_outputs:
            cmd += (UPYTHON_PROMPT)

        exp_output = [EXP_UPYTHON_HEADER, ]
        for cmd, out in zip(program_cmd, cmd_outputs):
            exp_output += cmd + '\r\n' + out + UPYTHON_PROMPT
        exp_output = "".join(exp_output)

        result = MicropythonMock().get_output(MicropythonReplHarness().harness,
                                              TestReplHarness.harness_callback,
                                              test_path, program_cmd, cmd_outputs, exp_output)

        check_result(result, TestResult(test_path, TestResult.PASS))

    def test_repl_incorrect_result(self):
        test_path = "/usr/test/micropython/test.py"

        program_cmd = ['2+2', ]
        cmd_outputs = [f'4{UPYTHON_PROMPT}', ]
        exp_output = EXP_UPYTHON_HEADER + program_cmd[0] + '\r\n' + "5" + UPYTHON_PROMPT

        result = MicropythonMock().get_output(MicropythonReplHarness().harness,
                                              TestReplHarness.harness_callback,
                                              test_path, program_cmd, cmd_outputs, exp_output)

        exp_result = TestResult(test_path,
                                TestResult.FAIL,
                                None)

        check_result(result, exp_result)
