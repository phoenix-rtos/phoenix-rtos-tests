# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# Concatenate /etc/shells file to stdout using psh cat command
#
# Copyright 2021, 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh


@psh.run
def harness(p):
    psh.assert_cmd(p, 'cat etc/shells',
                   expected=r'# /etc/shells: valid login shells(\r+)\n/bin/sh(\r+)\n',
                   msg='The /etc/shells/ file content is invalid',
                   is_regex=True)
