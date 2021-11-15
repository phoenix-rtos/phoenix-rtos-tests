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


def harness(p):
    psh.init(p)

    # need to add more test cases when it will be possible to write content to the file
    assert_cat_err(p)
    assert_cat_h(p)
