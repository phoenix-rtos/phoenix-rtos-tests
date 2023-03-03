import re
from pathlib import Path

from trunner.types import Status, TestResult

PROMPT = r"(\r+)\x1b\[0J" + r"\(psh\)% "
EOL = r"\r+\n"

UPYTH_PROMPT = ">>>"
MICROPYTHON = "/bin/micropython"

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


def is_test_with_regex_output(test_path: str):
    """Some tests needs special handling - their expected output is a regex, not just string"""

    p = Path(test_path)
    return p.parent.name + "/" + p.name in TESTS_WITH_REGEX_OUTPUT


def get_test(dut):
    dut.expect(PROMPT, timeout=45)
    output = dut.before
    # Leave prompt in the buffer
    dut.sendline()

    re_result = re.search(r"Error: (.+) - (.+)", output)
    assert (
        re_result is None
    ), f"There was an error {re_result.group(2)} during program {re_result.group(1)} execution:\n{output}\n"

    # Looking for a path to the test
    re_result = re.search(r"Running test: (/.+\.py)", output)
    assert re_result is not None, "Can't find path to the test\nProgram output:\n%s\n" % (output)

    test_path = re_result.group(1)

    # After the path there is a program result
    index = output.find("\n")
    test_result = output[index + 1:]

    return test_path, test_result


def get_exp_output(dut, test_path: str):
    cmd = f"cat {test_path}.exp"

    dut.expect(PROMPT)
    dut.sendline(cmd)
    dut.expect(cmd + EOL)
    dut.expect(PROMPT)
    output = dut.before
    # Leave prompt in the buffer
    dut.sendline()

    return output


def harness(dut):
    """Harness for getting test result from MicroPython test and comparing to the expected output"""

    test_path, test_result = get_test(dut)

    if re.match("SKIP", test_result):
        res = TestResult(status=Status.SKIP)
        dut.sendline()
        return res

    exp_output = get_exp_output(dut, test_path)

    test_passed = False
    if is_test_with_regex_output(test_path):
        expected_output_re = create_regex(exp_output)
        if re.match(expected_output_re, test_result):
            test_passed = True
    elif exp_output == test_result:
        test_passed = True

    if test_passed:
        return TestResult(status=Status.OK)
    else:
        msg = f"Incorrect result!\n\nExpected result:\n{exp_output}\nTest result:\n{test_result}\n"
        return TestResult(msg=msg, status=Status.FAIL)
