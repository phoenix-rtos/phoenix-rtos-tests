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

import psh.tools.psh as psh
from trunner.config import CURRENT_TARGET


def assert_cat_err(p):
    fname = 'nonexistentFile'
    cmd = f'cat {fname}'
    statement = f'cat: {fname} no such file'

    psh.assert_cmd(p, cmd, expected=statement)


def assert_cat_h(p):
    cmd = 'cat -h'
    help = ('Usage: cat [options] [files]',
            '  -h:  shows this help message')

    psh.assert_cmd(p, cmd, expected=help)


def assert_cat_shells(p):
    fname = 'etc/shells'
    fcontent = r'# /etc/shells: valid login shells(\r+)\n/bin/sh'
    cmd = f'cat {fname}'

    psh.assert_cmd(p, cmd, expected=fcontent, msg='The /etc/shells/ file content is invalid', is_regex=True)


def harness(p):
    psh.init(p)

    # need to add more test cases when it will be possible to write content to the file
    assert_cat_err(p)
    assert_cat_h(p)

    # only on ia32-generic target are files that can be written out
    if CURRENT_TARGET == 'ia32-generic':
        assert_cat_shells(p)
