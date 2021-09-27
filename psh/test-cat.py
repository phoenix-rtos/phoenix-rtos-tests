# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh cat command test
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

from psh.tools.basic import run_psh, assert_only_prompt

PROMPT = r'\r\x1b\[0J' + r'\(psh\)% '
EOL = r'(\r+)\n'


def is_hardware_target(p):
    cmd = 'ls'

    p.sendline(cmd)
    p.expect(cmd)
    idx = p.expect([
            'syspage',
            'bin'])
    p.expect(PROMPT)

    if idx == 0:
        return True
    elif idx == 1:
        return False


def assert_cat_err(p):
    fname = 'nonexistentFile'
    cmd = f'cat {fname}'
    statement = f'cat: {fname} no such file'

    p.sendline(cmd)
    p.expect(cmd + EOL + statement + EOL + PROMPT)


def assert_cat_h(p):
    cmd = 'cat -h'
    help = r'Usage: cat \[options\] \[files\](\r+)\n' \
        + r'  -h:  shows this help message'

    p.sendline(cmd)
    p.expect(cmd + EOL + help + EOL + PROMPT)


def assert_cat_shells(p):
    fname = 'etc/shells'
    fcontent = '# /etc/shells: valid login shells(\r+)\n/bin/sh'
    cmd = f'cat {fname}'

    p.sendline(cmd)
    p.expect(cmd + EOL + fcontent + EOL + PROMPT)


def harness(p):

    run_psh(p)
    assert_only_prompt(p)

    # need to add more test cases when it will be possible to write content to the file
    assert_cat_err(p)
    assert_cat_h(p)

    # there are no files on imxrt106x target that can be written out
    if not is_hardware_target(p):
        assert_cat_shells(p)
