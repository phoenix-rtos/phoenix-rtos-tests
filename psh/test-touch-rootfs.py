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


def assert_hardlinks(p):
    dates = {}
    # all psh commands are hardlinks
    for hardlink in psh.get_commands(p):
        hardlink_path = f'/bin/{hardlink}'
        dates[hardlink] = psh.date(p)
        msg = f"Prompt hasn't been seen after the hardlink touch: {hardlink_path}"
        psh.assert_cmd(p, f'touch {hardlink_path}', msg=msg)

    assert_mtime(p, dates, '/bin')


@psh.run
def harness(p):
    assert_hardlinks(p)
