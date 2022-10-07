'''psh module with tools for psh related tests'''
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

import re
import pexpect

import trunner.config as config


EOL = r'(\r+)\n'
PROMPT = r'(\r+)\x1b\[0J' + r'\(psh\)% '


def init(pexpect_proc):
    ''' Runs psh and asserts a first prompt'''
    run(pexpect_proc)
    assert_only_prompt(pexpect_proc)


def _readable(exp_regex):
    ''' Gets more readable expected regex with new lines instead of raw string EOL,
    and prompt without raw esc sequences. '''
    exp_regex = exp_regex.replace(EOL, '\n')
    return exp_regex.replace(PROMPT, '(psh)% ')


def assert_cmd(pexpect_proc, cmd, expected='', msg='', is_regex=False):
    ''' Sends specified command and asserts that it's displayed correctly
    with optional expected output and next prompt'''
    pexpect_proc.sendline(cmd)
    cmd = re.escape(cmd)
    exp_regex = ''
    if is_regex:
        exp_regex = expected
    elif expected != '':
        if not (isinstance(expected, tuple) or isinstance(expected, list)):
            expected = tuple((expected,))
        for line in expected:
            line = re.escape(line)
            exp_regex += line + EOL

    exp_regex = cmd + EOL + exp_regex + PROMPT
    exp_readable = _readable(exp_regex)
    msg = f'Expected output regex was: \n---\n{exp_readable}\n---\n' + msg
    assert pexpect_proc.expect([exp_regex, pexpect.TIMEOUT, pexpect.EOF]) == 0, msg


def assert_prompt_after_cmd(pexpect_proc, cmd, msg=''):
    pexpect_proc.sendline(cmd)
    exp_regex = EOL + PROMPT
    assert pexpect_proc.expect([exp_regex, pexpect.TIMEOUT]) == 0, msg

    return pexpect_proc.before


def assert_only_prompt(pexpect_proc):
    ''' Expect an erase in display ascii escape sequence and a prompt sign '''
    prompt = '\r\x1b[0J' + '(psh)% '
    got = pexpect_proc.read(len(prompt))
    assert got == prompt, f'Expected:\n{prompt}\nGot:\n{got}'


def assert_prompt(pexpect_proc, msg='', timeout=-1, catch_timeout=True):
    patterns = ['(psh)% ']
    if catch_timeout:
        patterns.append(pexpect.TIMEOUT)

    idx = pexpect_proc.expect_exact(patterns, timeout=timeout)
    # if catch_timeout is false then pyexpect exception is raised
    assert idx == 0, msg


def assert_prompt_fail(pexpect_proc, msg='', timeout=-1):
    patterns = ['(psh)% ', pexpect.TIMEOUT, pexpect.EOF]
    idx = pexpect_proc.expect_exact(patterns, timeout=timeout)
    assert idx != 0, msg


def assert_exec(pexpect_proc, prog, expected='', msg=''):
    ''' Executes specified program and asserts that it's displayed correctly
    with optional expected output and next prompt'''

    if config.CURRENT_TARGET in config.SYSEXEC_TARGETS:
        exec_cmd = f'sysexec {prog}'
    else:
        exec_cmd = f'/bin/{prog}'

    assert_cmd(pexpect_proc, exec_cmd, expected, msg)


def _get_exit_code(pexpect_proc):
    pexpect_proc.sendline('echo $?')
    msg = 'Checking passed command return code failed, one or more digits not found'
    assert pexpect_proc.expect([rf'(\d+?){EOL}', pexpect.TIMEOUT, pexpect.EOF]) == 0, msg

    exit_code = int(pexpect_proc.match.group(1))
    assert_only_prompt(pexpect_proc)

    return exit_code


def assert_cmd_failed(pexpect_proc, ):
    assert _get_exit_code(pexpect_proc) != 0, 'The exit status of last passed command equals 0!'


def assert_cmd_successed(pexpect_proc):
    assert _get_exit_code(pexpect_proc) == 0, 'The exit status of last passed command does not equal 0!'


def get_commands(pexpect_proc):
    ''' Returns a list of available psh commands'''
    commands = []
    pexpect_proc.sendline('help')
    idx = pexpect_proc.expect_exact(['help', pexpect.TIMEOUT])
    assert idx == 0, "help command hasn't been sent properly"

    while idx != 1:
        idx = pexpect_proc.expect([r'(\w+)(\s+-.*?\n)', r'\(psh\)% '])
        if idx == 0:
            groups = pexpect_proc.match.groups()
            commands.append(groups[0])

    return commands


def run(pexpect_proc):
    pexpect_proc.send('psh\r\n')
    pexpect_proc.expect(r'psh(\r+)\n')
