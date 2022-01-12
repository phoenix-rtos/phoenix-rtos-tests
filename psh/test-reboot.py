# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# simple psh reboot command test
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh


def harness(p):
    psh.init(p)

    p.sendline('reboot')
    p.expect_exact('reboot')

    msg = 'Prompt not seen 8 seconds after reboot'
    psh.assert_prompt(p, msg, timeout=8)
