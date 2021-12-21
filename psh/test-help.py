# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh help command test
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import pexpect
import psh.tools.psh as psh


def harness(p):
    psh.init(p)

    p.sendline('help')
    idx = p.expect_exact(['help', pexpect.TIMEOUT, pexpect.EOF])
    assert idx == 0, "help command hasn't been sent properly"
    # asserts that at least 15 help commands are displayed in proper format
    idx = p.expect([r'((\s*)(\w+)(\s+)-(\s\w+)+(\s*)(\n)){15,}', pexpect.TIMEOUT, pexpect.EOF])
    assert idx == 0, "Wrong help message format"
