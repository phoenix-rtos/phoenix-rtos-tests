import re

from trunner.types import Status, TestResult
from trunner.text import bold
from trunner.dut import Dut

import psh.tools.psh as psh

PROMPT = r"(\r*)\x1b\[0J" + r"\(psh\)% "
EOL = r"(?:\r+)\n"
UPYTH_PROMPT = ">>>"
MICROPYTHON = "/bin/micropython "


def standard_regex_escape(text):
    """In tests with regex output syntax character \\ makes next char to be interpreted as a regex character"""

    result = []
    escape = False
    for c in text:
        if c == "\r":
            continue
        if c == "\n":
            result.append(r"(?:\r+)")
            result.append("\n")
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


@psh.run
def harness(dut: Dut, **kwargs):
    """Harness for testing MicroPython REPL"""

    args = kwargs.get("args", "")
    test_path = kwargs.get("path")
    assert test_path is not None

    # sending cat with path to the test to get commands to insert in micropython terminal
    cmd = "cat " + test_path
    dut.sendline(cmd)
    dut.expect(cmd + EOL)

    dut.expect(PROMPT)
    script = dut.before
    script = script.replace("        ", "\t")  # workaround
    script = re.split(EOL, script)

    # sending cat with path to the expected output from test
    dut.sendline(cmd + ".exp")
    dut.expect(cmd + ".exp" + EOL)
    dut.expect(PROMPT)

    # remove trailing characters
    exp_output = dut.before[:-3]

    # start micropython interpreter
    dut.sendline(MICROPYTHON + args)
    dut.expect(MICROPYTHON + standard_regex_escape(args) + EOL)

    test_output = []

    for line in script:
        prompts = [">>>", "..."]
        idx = dut.expect_exact(prompts)
        test_output.append(dut.before + prompts[idx])
        dut.sendline(line)

    dut.expect_exact(UPYTH_PROMPT)
    # exit MicroPython REPL
    dut.sendline("\x04")
    dut.expect(PROMPT)

    test_output = "".join(test_output)
    expected_output_re = create_regex(exp_output)

    if re.match(expected_output_re, test_output):
        return TestResult(status=Status.OK)
    else:
        msg = f"{bold('INCORRECT OUTPUT')}\n\n{bold('EXPECTED RESULT:')}\n{expected_output_re}"
        msg += f"\n\n{bold('TEST RESULT:')}\n{test_output}"
        return TestResult(msg=msg, status=Status.FAIL)
