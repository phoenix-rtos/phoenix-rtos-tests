# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh text editor test
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

from psh.tools.common import CHARS, assert_created, assert_random
import psh.tools.psh as psh


def harness(p):
    testdir_name = 'test_touch_dir'
    psh.init(p)

    assert_created(p, 'mkdir', testdir_name)
    assert_created(p, 'touch', f'{testdir_name}/test_file')
    # double touch the same file should have the same effect
    assert_created(p, 'touch', f'{testdir_name}/test_file')
    assert_created(p, 'touch', f'{testdir_name}/' + ''.join(CHARS))
    assert_random(p, CHARS, 'touch', f'{testdir_name}/test_touch_random', count=20)
