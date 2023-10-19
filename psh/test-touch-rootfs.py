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
from psh.tools.common import assert_dir_mtimes


def assert_bin(p):
    ''' Asserts that all files in /bin are able to be touched'''
    dates = {}
    # Due to the long execution time on zynq-qemu target (especially during CI) after reboot timeout is increased
    # https://github.com/phoenix-rtos/phoenix-rtos-project/issues/893
    for file_info in psh.ls(p, 'bin', timeout=40):
        # If file has multiple links, dates of all linked files will change when one is touched
        if file_info.n_links > 1:
            continue

        file = file_info.name
        file_path = f'/bin/{file}'
        dates[file] = psh.date(p)
        msg = f"Prompt hasn't been seen after touching the following file: {file_path}"
        psh.assert_cmd(p, f'touch {file_path}', result='success', msg=msg)

    assert_dir_mtimes(p, dates, '/bin')


@psh.run
def harness(p):
    assert_bin(p)
