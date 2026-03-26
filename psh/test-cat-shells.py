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

from trunner import ctx

import psh.tools.psh as psh


# TODO switch to new test harness template providing ctx in args
@psh.run
def harness(p):
    # On NOMMU targets shells file is loaded to syspage
    dir = "syspage" if not ctx.target.rootfs else "etc"
    psh.assert_cmd(p, f'cat {dir}/shells',
                   expected=r'# /etc/shells: valid login shells(\r+)\n/bin/sh(\r+)\n',
                   msg=f'The /{dir}/shells/ file content is invalid',
                   is_regex=True)
