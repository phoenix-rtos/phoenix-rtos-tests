
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

def run_psh(p):
    p.send('psh\r\n')
    p.expect(r'psh(\r+)\n')


def assert_only_prompt(p):
    # Expect an erase in display ascii escape sequence and a prompt sign
    prompt = '\r\x1b[0J' + '(psh)% '
    got = p.read(len(prompt))
    assert got == prompt, f'Expected:\n{prompt}\nGot:\n{got}'
