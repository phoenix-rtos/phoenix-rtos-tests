# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# simple harness test asserting test_priority executable
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

    psh.assert_exec(p, 'test_priority', '', 'Prompt not seen after test_priority execution')
    psh.assert_cmd(p, 'echo $?', '0', "Exit code from test_priority wasn't 0!")
