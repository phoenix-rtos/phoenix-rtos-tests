import re

from trunner.types import Status, TestResult

PROMPT = r"(\r+)\x1b\[0J" + r"\(psh\)% "
EOL = r"\r+\n"
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


def send_micropython_cmd(dut, cmd: str):
    dut.sendline(cmd)
    dut.expect(UPYTH_PROMPT)

    return dut.before + UPYTH_PROMPT


# TODO this test does not work
def harness(dut):
    """Harness for testing MicroPython REPL"""

    # At first echo prints path to the test
    dut.expect(PROMPT)
    test_path = dut.before.rstrip()

    # sending cat with path to the test to get commands to insert in micropython terminal
    cmd = "cat " + test_path
    dut.sendline(cmd)
    dut.expect(cmd + EOL)
    dut.expect(PROMPT)

    script = dut.before
    script = re.split(EOL, script)
    script = script[:-1]  # remove empty string at the end

    # sending cat with path to the expected output from test
    dut.sendline(cmd + ".exp")
    dut.expect(cmd + ".exp" + EOL)
    dut.expect(PROMPT)

    exp_output = dut.before

    # Start micropython interpreter
    dut.sendline(MICROPYTHON)
    dut.expect(MICROPYTHON + EOL)

    test_output = []

    for line in script:
        prompts = [">>>", "..."]
        idx = dut.expect_exact(prompts)
        test_output.append(dut.before + prompts[idx])
        dut.sendline(line)

    dut.sendcontrol("d")
    dut.expect(PROMPT)

    test_output.append(dut.before)
    test_output = "".join(test_output)
    expected_output_re = create_regex(exp_output)

    if re.match(expected_output_re, test_output):
        return TestResult(status=Status.OK)
    else:
        msg = f"Incorrect output\n\nExpected result:\n{expected_output_re}\nTest result:\n{test_output}"
        return TestResult(msg=msg, status=Status.FAIL)
