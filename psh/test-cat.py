# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh cat command test
#
# Copyright 2021, 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh


def assert_cat_err(p):
    fname = 'nonexistentFile'
    statement = rf'cat: {fname}(.+)'
    psh.assert_cmd(p, f'cat {fname}', expected=statement, result='fail', is_regex=True)


def assert_cat_h(p):
    help = ('Usage: cat [options] [files]',
            '  -h:  shows this help message')
    psh.assert_cmd(p, 'cat -h', expected=help, result='success')


@psh.run
def harness(p):
    # need to add more test cases when it will be possible to write content to the file
    assert_cat_err(p)
    assert_cat_h(p)
