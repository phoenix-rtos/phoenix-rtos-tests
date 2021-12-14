'''psh module with tools for psh related tests'''
# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh module with tools for psh related tests
#
# Copyright 2021 Phoenix Systems
# Author: Jakub Sarzyński, Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import re
import pexpect


from trunner.config import CURRENT_TARGET, DEVICE_TARGETS
from collections import namedtuple

EOL = r'(\r+)\n'
PROMPT = r'(\r+)\x1b\[0J' + r'\(psh\)% '


def init(pexpect_proc):
    ''' Runs psh and asserts a first prompt'''
    assert_exec(pexpect_proc, 'psh')
    # run(pexpect_proc)
    # assert_only_prompt(pexpect_proc)


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
    if expected != '':
        if type(expected) is tuple:
            for line in expected:
                if not is_regex:
                    line = re.escape(line)
                exp_regex = exp_regex + line + EOL
        else:
            exp_regex = expected
            if not is_regex:
                exp_regex = re.escape(exp_regex)
            exp_regex = exp_regex + EOL

    exp_regex = cmd + EOL + exp_regex + PROMPT
    exp_readable = _readable(exp_regex)
    msg = f'Expected output regex was: \n---\n{exp_readable}\n---\n' + msg
    assert pexpect_proc.expect([exp_regex, pexpect.TIMEOUT]) == 0, msg


def ls(p, dir=''):
    ''' Returns the named tuple with information about files present in the specified directory '''
    File = namedtuple('File', ['name', 'owner', 'is_dir'])

    p.sendline(f'ls -la {dir}')
    p.expect_exact('ls -la')
    if dir:
        p.expect_exact(f' {dir}')

    p.expect_exact('\n')

    files = []
    while True:
        idx = p.expect([r'\(psh\)\% ', r'(.*?)(\r+)\n'])
        if idx == 0:
            break

        line = p.match.group(0)

        try:
            permissions, _, owner, _, _, _, _, _, name = line.split()
        except ValueError:
            assert False, f'wrong ls output: {line}'

        # Name is printed with ascii escaped characters - remove them
        if name.startswith('\x1b[34m'):
            name = name[5:]
        if name.endswith('\x1b[0m'):
            name = name[:-4]

        f = File(name, owner, permissions[0] == 'd')
        files.append(f)

    return files


def assert_unprintable(pexpect_proc, cmd, msg=''):
    pexpect_proc.sendline(cmd)
    exp_regex = EOL + PROMPT
    assert pexpect_proc.expect([exp_regex, pexpect.TIMEOUT]) == 0, msg


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
    patterns = ['(psh)% ', pexpect.TIMEOUT]
    idx = pexpect_proc.expect_exact(patterns, timeout=timeout)
    assert idx == 1, msg


def assert_exec(pexpect_proc, prog, expected='', msg=''):
    ''' Executes specified program and asserts that it's displayed correctly
    with optional expected output and next prompt'''
    if CURRENT_TARGET in DEVICE_TARGETS:
        exec_cmd = f'sysexec {prog}'
    else:
        exec_cmd = f'/bin/{prog}'

    assert_cmd(pexpect_proc, exec_cmd, expected, msg)


def run(pexpect_proc):
    pexpect_proc.send('psh\r\n')
    pexpect_proc.expect(r'psh(\r+)\n')
