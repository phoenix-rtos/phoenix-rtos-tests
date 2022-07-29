#
# Phoenix-RTOS test runner
#
# The harness for the MicroPython Test Suite
#
# Copyright 2022 Phoenix Systems
# Authors: Piotr Nieciecki
#

import pathlib
import re

from .common import TestResult

PROMPT = r'(\r+)\x1b\[0J' + r'\(psh\)% '
EOL = r'(\r+)\n'

UPYTH_PROMPT = ">>>"
MICROPYTHON = "/bin/micropython"


def standard_regex_escape(text):
    """In tests with regex output syntax character \\ makes next char to be interpreted as a regex character"""

    result = []
    escape = False
    for c in text:
        if c == "\r":
            continue
        if c == "\n":
            result.append(EOL)
        elif escape:
            escape = False
            result.append(c)
        elif c == "\\":
            escape = True
        elif c in ("(", ")", "[", "]", "{", "}", ".", "*", "+", "^", "$", "?", "|"):
            result.append("\\" + c)
        else:
            result.append(c)

    return "".join(result)


def create_regex(text):
    """Tests with regex output use special syntax for expected outputs"""

    text = standard_regex_escape(text)

    # ######## matches zero or more lines with text
    text = text.replace(f"########{EOL}.+", "(.|\n)*")
    text = text.replace(f"{EOL}########", "(.|\n)*")
    text = text.replace("########", "(.|\n)*")

    return text


class MicropythonStandardHarness:
    """Class providing harness for getting test result from MicroPython test and comparing to the expected output"""

    TESTS_WITH_REGEX_OUTPUT = {
        "micropython/meminfo.py",
        "basics/bytes_compare3.py",
        "basics/builtin_help.py",
        "thread/thread_exc2.py",
        "esp32/partition_ota.py",
        "cmdline/cmd_parsetree.py",
        "cmdline/cmd_showbc.py",
        "cmdline/cmd_verbose.py",
    }

    def is_test_with_regex_output(self, path_to_test):
        """Some tests needs special handling - their expected output is a regex, not just string"""

        p = pathlib.Path(path_to_test)
        return p.parent.name + '/' + p.name in self.TESTS_WITH_REGEX_OUTPUT

    def get_test(self, proc):
        proc.expect(PROMPT, timeout=45)
        output = proc.before

        re_result = re.search(r"Error: (.+) - (.+)", output)
        assert re_result is None, \
            f"There was an error {re_result.group(2)} during program {re_result.group(1)} execution:\n{output}\n"

        # Looking for a path to the test
        re_result = re.search(r'Running test: (/.+\.py)', output)
        assert re_result is not None, "Can't find path to the test\nProgram output:\n%s\n" % (output)

        test_path = re_result.group(1)

        # After the path there is a program result
        index = output.find('\n')
        test_result = output[index+1:]

        return test_path, test_result

    def get_exp_output(self, test_path, proc):
        cmd = f"cat {test_path}.exp"

        proc.sendline(cmd)
        proc.expect(cmd + EOL)
        proc.expect(PROMPT)

        return proc.before

    def harness(self, p):
        test_output = []

        test_path, test_result = self.get_test(p)

        if re.match(r'SKIP', test_result):
            test_output.append(TestResult(test_path, TestResult.IGNORE,
                               "Test uses modules, which are disabled in our implementation"))
            return test_output

        exp_output = self.get_exp_output(test_path, p)

        test_passed = False
        if self.is_test_with_regex_output(test_path):
            expected_output_re = create_regex(exp_output)
            if re.match(expected_output_re, test_result):
                test_passed = True
        elif exp_output == test_result:
            test_passed = True

        if test_passed:
            test_output.append(TestResult(test_path, TestResult.PASS))
        else:
            test_output.append(TestResult(test_path, TestResult.FAIL,
                               f"Incorrect result!\n\nExpected result:\n{exp_output}\nTest result:\n{test_result}\n"))

        return test_output
