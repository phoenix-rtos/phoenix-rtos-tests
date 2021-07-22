
# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# basic tools for psh related tests
#
# Copyright 2021 Phoenix Systems
# Author: Jakub Sarzyński
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import pexpect


def run_psh(p):
    p.send('psh\r\n')
    p.expect(r'psh(\r+)\n')


def assert_only_prompt(p):
    # Expect an erase in display ascii escape sequence and a prompt sign
    prompt = '\r\x1b[0J' + '(psh)% '
    got = p.read(len(prompt))
    assert got == prompt, f'Expected:\n{prompt}\nGot:\n{got}'


def assert_prompt(p, msg=None, timeout=-1, catch_timeout=True):
    if not msg:
        msg = ''

    patterns = ['(psh)% ']
    if catch_timeout:
        patterns.append(pexpect.TIMEOUT)

    idx = p.expect_exact(patterns, timeout=timeout)
    # if catch_timeout is false then pyexpect exception is raised
    assert idx == 0, msg


def assert_prompt_fail(p, msg=None, timeout=-1):
    if not msg:
        msg = ''

    patterns = ['(psh)% ', pexpect.TIMEOUT]
    idx = p.expect_exact(patterns, timeout=timeout)
    assert idx == 1, msg
