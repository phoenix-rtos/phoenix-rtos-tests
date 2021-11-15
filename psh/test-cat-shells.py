# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# Concatenate /etc/shells file to stdout using psh cat command
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh


def harness(p):
    psh.init(p)

    fname = 'etc/shells'
    fcontent = r'# /etc/shells: valid login shells(\r+)\n/bin/sh'
    cmd = f'cat {fname}'

    psh.assert_cmd(p, cmd, expected=fcontent, msg='The /etc/shells/ file content is invalid', is_regex=True)
