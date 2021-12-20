# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh exit command test
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

    psh.assert_exec(p, 'psh', msg="Prompt hasn't been seen when creating new psh process using runfile")
    psh.assert_cmd(p, 'exit', msg="Prompt hasn't been seen after first exit (two psh processes should be run)")
    p.sendline('exit')
    p.expect_exact('exit')
    psh.assert_prompt_fail(p, 'Prompt has been seen after sending `exit` command', timeout=1)
