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


@psh.run
def harness(p):
    psh.assert_exec(p, 'test_priority', msg='Prompt not seen after test_priority execution')
