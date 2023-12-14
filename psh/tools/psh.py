"""psh module with tools for psh related tests"""
# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh module with tools for psh related tests
#
# Copyright 2021, 2022 Phoenix Systems
# Author: Jakub Sarzyński, Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import functools
import re
from collections import namedtuple

import pexpect

import trunner

from datetime import datetime, timedelta

EOL = r"(?:\r+)\n"
EOT = "\x04"
PROMPT = r"(\r*)\x1b\[0J" + r"\(psh\)% "
# according to control sequence introducer pattern
CONTROL_CODE = r"(\x1b\[[\x30-\x3F]*[\x20-\x2F]*[\x40-\x7E])"
OPTIONAL_CONTROL_CODE = CONTROL_CODE + r"?"


File = namedtuple("File", ["name", "owner", "is_dir", "datetime", "datetime_resolution", "n_links"])


def _readable(exp_regex):
    """Gets more readable expected regex with new lines instead of raw string EOL,
    and prompt without raw esc sequences."""
    exp_regex = exp_regex.replace(EOL, "\n")
    return exp_regex.replace(PROMPT, "(psh)% ")


def _check_result(pexpect_proc, result):
    if result == "dont-check":
        return
    elif result == "success":
        assert_cmd_successed(pexpect_proc)
    elif result == "fail":
        assert_cmd_failed(pexpect_proc)
    else:
        raise Exception(
            f"The value {result} for `result` argument is not correct. "
            "Please choose between susccess/fail/dont-check'."
        )


def assert_cmd(pexpect_proc, cmd, *, expected="", result="success", msg="", is_regex=False, timeout=-1):
    """Sends specified command and asserts that it's displayed correctly
    with optional expected output and next prompt. Exit status is asserted depending on `result`"""
    pexpect_proc.sendline(cmd)
    cmd = re.escape(cmd)
    exp_regex = ""
    if is_regex:
        exp_regex = expected
    elif expected != "":
        if not (isinstance(expected, tuple) or isinstance(expected, list)):
            expected = tuple((expected,))
        for line in expected:
            line = re.escape(line)
            exp_regex += line + EOL

    exp_regex = cmd + EOL + exp_regex + PROMPT
    exp_readable = _readable(exp_regex)
    msg = f"Expected output regex was: \n---\n{exp_readable}\n---\n" + msg

    assert pexpect_proc.expect([exp_regex, pexpect.TIMEOUT, pexpect.EOF], timeout=timeout) == 0, msg

    _check_result(pexpect_proc, result)


def assert_prompt_after_cmd(pexpect_proc, cmd, result="success", msg=None):
    """Sends specified command and asserts that the command and next prompt are displayed correctly.
    Exit status is asserted depending on the result argument"""
    pexpect_proc.sendline(cmd)
    exp_regex = EOL + PROMPT
    if not msg:
        msg = f"Prompt not seen after sending the following command: {cmd}"
    assert pexpect_proc.expect([exp_regex, pexpect.TIMEOUT]) == 0, msg
    output = pexpect_proc.before

    _check_result(pexpect_proc, result)

    return output


def assert_prompt(pexpect_proc, msg="", timeout=-1, catch_timeout=True):
    patterns = ["(psh)% "]
    if catch_timeout:
        patterns.append(pexpect.TIMEOUT)

    idx = pexpect_proc.expect_exact(patterns, timeout=timeout)
    # if catch_timeout is false then pyexpect exception is raised
    assert idx == 0, msg


def assert_prompt_fail(pexpect_proc, msg="", timeout=-1):
    patterns = ["(psh)% ", pexpect.TIMEOUT, pexpect.EOF]
    idx = pexpect_proc.expect_exact(patterns, timeout=timeout)
    assert idx != 0, msg


def _get_exec_cmd(prog):
    if trunner.ctx.target.rootfs:
        return f"/bin/{prog}"
    else:
        return f"sysexec {prog}"


def assert_exec(pexpect_proc, prog, expected="", msg=""):
    """Executes specified program and asserts that it's displayed correctly
    with optional expected output and next prompt"""
    exec_cmd = _get_exec_cmd(prog)

    assert_cmd(pexpect_proc, exec_cmd, expected=expected, result="success", msg=msg)


def get_exit_code(pexpect_proc):
    pexpect_proc.sendline("echo $?")
    msg = "Checking passed command return code failed, one or more digits not found"
    assert pexpect_proc.expect([rf"(\d+?){EOL}", pexpect.TIMEOUT, pexpect.EOF]) == 0, msg

    exit_code = int(pexpect_proc.match.group(1))
    assert_prompt(pexpect_proc)

    return exit_code


def assert_cmd_failed(pexpect_proc):
    assert get_exit_code(pexpect_proc) != 0, "The exit status of last passed command equals 0!"


def assert_cmd_successed(pexpect_proc):
    assert get_exit_code(pexpect_proc) == 0, "The exit status of last passed command does not equal 0!"


def _send(pexpect_proc, cmd):
    cmd.strip()
    pexpect_proc.sendline(cmd)
    idx = pexpect_proc.expect_exact([cmd, pexpect.TIMEOUT, pexpect.EOF])
    assert idx == 0, f"{cmd} command hasn't been sent properly"


def uptime(pexpect_proc):
    """Returns tuple with time since start of a system in format: hh:mm:ss"""
    Time = namedtuple("Time", ["hour", "minute", "second"])
    _send(pexpect_proc, "uptime")

    idx = 0

    groups = None
    while idx != 1:
        idx = pexpect_proc.expect(
            [
                r"up (\d+):(\d+):(\d+).*?\n",
                r"\(psh\)\% ",
            ]
        )
        if idx == 0:
            groups = pexpect_proc.match.groups()

    assert groups is not None
    hour, minute, second = groups
    time = Time(hour, minute, second)

    return time


def date(pexpect_proc):
    """Returns the system date in a datetime object"""
    _send(pexpect_proc, "date +%Y:%m:%d:%H:%M:%S")
    pexpect_proc.expect(rf"(?P<year>\d+):(?P<month>\d+):(?P<day>\d+):(?P<hour>\d+):(?P<min>\d+):(?P<sec>\d+){EOL}")

    m = pexpect_proc.match

    assert_prompt(pexpect_proc)
    return datetime(*map(int, m.groups()))


def ls(pexpect_proc, dir="", timeout=-1):
    """Returns the list with named tuples containing information about files present in the specified directory
    Alternatively, the list with only 1 file is returned providing file path has been passed."""

    current_year = date(pexpect_proc).year
    _send(pexpect_proc, f"ls -la {dir}")

    pexpect_proc.expect_exact("\n")

    files = []
    while True:
        idx = pexpect_proc.expect([r"\(psh\)\% ", r"([^\r\n]+(\r+\n))"], timeout=timeout)
        if idx == 0:
            break

        line = pexpect_proc.match.group(0)
        # temporary solution to match line with missing `---` in owner position (issue #254)
        line = line.replace("    ---", "--- ---")
        try:
            permissions, n_links_str, owner, _, _, month, mday, time_year, name = line.split(maxsplit=9)
        except ValueError:
            assert False, f"wrong ls output: {line}"

        # Name is printed with ascii escape characters - remove them
        name = re.sub(CONTROL_CODE, "", name)

        # time_year can contain either time (HH:MM) or year (YYYY)
        # If year was not printed, replace it with current year
        if ":" in time_year:
            time = time_year
            year = current_year
            resolution = timedelta(minutes=1)
        else:
            time = "00:00"
            year = time_year
            resolution = timedelta(days=1)

        file_datetime = datetime.strptime(f"{year} {month} {mday} {time}:00", "%Y %b %d %X")

        f = File(name, owner, permissions[0] == "d", file_datetime, resolution, int(n_links_str))
        files.append(f)

    return files


def ls_simple(pexpect_proc, dir=""):
    """Returns list of file names from the specified directory"""
    files = []
    _send(pexpect_proc, f"ls -1 {dir}")

    while True:
        idx = pexpect_proc.expect([r"\(psh\)\% ", r"((?P<fname>[^\r\n]+)(\r+\n))"])
        if idx == 0:
            break
        else:
            file = pexpect_proc.match.group("fname")
            # Name is printed with ascii escape characters - remove them
            esc_sequences = re.findall(CONTROL_CODE, file)
            for seq in esc_sequences:
                file = file.replace(seq, "")

            files.append(file)

    return files


def get_commands(pexpect_proc):
    """Returns a list of available psh commands"""
    commands = []
    pexpect_proc.sendline("help")
    idx = pexpect_proc.expect_exact(["help", pexpect.TIMEOUT])
    assert idx == 0, "help command hasn't been sent properly"

    while idx != 1:
        idx = pexpect_proc.expect([r"(\w+)(\s+-.*?\n)", r"\(psh\)% "])
        if idx == 0:
            groups = pexpect_proc.match.groups()
            commands.append(groups[0])

    return commands


def init(pexpect_proc):
    """Runs psh and asserts its first prompt"""
    # using runfile/sysexec to spawn a new psh process
    exec_cmd = _get_exec_cmd("psh")
    pexpect_proc.sendline(exec_cmd)
    pexpect_proc.expect(rf"{exec_cmd}(\r+)\n")
    assert_prompt(pexpect_proc)


def deinit(pexpect_proc):
    """Close spawned psh"""

    pexpect_proc.send(EOT)
    pexpect_proc.expect(r"exit(\r+)\n")


def run(harness):
    @functools.wraps(harness)
    def wrapper_harness(*args, **kwargs):
        dut = args[0]
        # to avoid CI fails caused by https://github.com/phoenix-rtos/phoenix-rtos-project/issues/866
        if trunner.ctx.target.name == "armv7m7-imxrt117x-evk":
            ret = harness(*args, **kwargs)
            # one prompt after a test should be left to be asserted by test runner
            dut.sendline("\n")
            return ret
        init(dut)
        res = harness(*args, **kwargs)
        deinit(dut)
        return res

    return wrapper_harness
