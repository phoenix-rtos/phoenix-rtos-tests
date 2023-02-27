# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh touch command test for rootfs targets
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%

import psh.tools.psh as psh
from psh.tools.common import assert_mtime


def assert_bin(p):
    ''' Asserts that all files in /bin are able to be touched'''
    dates = {}
    for file in psh.ls_simple(p, 'bin'):
        file_path = f'/bin/{file}'
        dates[file] = psh.date(p)
        msg = f"Prompt hasn't been seen after touching the following file: {file_path}"
        psh.assert_cmd(p, f'touch {file_path}', result='success', msg=msg)

    assert_mtime(p, dates, '/bin')


@psh.run
def harness(p):
    assert_bin(p)
