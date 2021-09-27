# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh gibbering input testing
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import string
import psh.tools.psh as psh
from trunner.config import CURRENT_TARGET


EXPECTED = 'Unknown command!'
EOT = '\x04'


def assert_letters(p):
    cmd = string.ascii_lowercase
    msg = 'Wrong output for lowercase letters'
    psh.assert_cmd(p, cmd, EXPECTED, msg)

    cmd = string.ascii_uppercase
    msg = 'Wrong output for uppercase letters'
    psh.assert_cmd(p, cmd, EXPECTED, msg)

    cmd = string.ascii_letters
    msg = 'Wrong output for all ascii letters'
    psh.assert_cmd(p, cmd, EXPECTED, msg)


def assert_digits(p):
    cmd = '0123456789'
    msg = 'Wrong output for all digits'
    psh.assert_cmd(p, cmd, EXPECTED, msg)


def assert_printable_specials(p):
    # `,\,",' characters should be removed after multi-lines implementation
    cmd = r'~!@#$%^&*()_+-=[];\',./:"<>?{}'
    msg = 'Wrong output for special characters'
    psh.assert_cmd(p, cmd, EXPECTED, msg)


def assert_unprintable_specials(p):
    for ascii_code in range(32):
        # ^C is terminating character for qemu, so we skip it
        if ascii_code == 3 and CURRENT_TARGET == 'ia32-generic':
            continue
        # ^D terminates psh process, so the another psh process is launched
        if ascii_code == 4:
            psh.assert_exec(p, 'psh')
        # FS character skipped, to prevent EOF in reader buffer
        if ascii_code == 28:
            continue
        cmd = chr(ascii_code)
        msg = f'Wrong output for character with the following ascii code: {ascii_code}'
        psh.assert_unprintable(p, cmd, msg)

        # LF or ^D, double prompt is expected
        if ascii_code == 4 or ascii_code == 10:
            psh.assert_prompt(p)

    # DEL character
    cmd = chr(127)
    msg = f'Wrong output for character with the following ascii code: {ascii_code}'
    psh.assert_unprintable(p, cmd, msg)


def assert_not_ascii(p):
    for code in range(128, 256):
        cmd = chr(code)
        msg = f'Wrong output for character with the following code: {code}'
        psh.assert_unprintable(p, cmd, msg)


def harness(p):
    psh.init(p)

    assert_letters(p)
    assert_digits(p)
    assert_printable_specials(p)
    assert_unprintable_specials(p)
    assert_not_ascii(p)
