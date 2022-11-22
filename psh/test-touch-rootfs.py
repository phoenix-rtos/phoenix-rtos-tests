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
from psh.tools.common import assert_timestamp_change


def assert_hardlinks(p):
    uptimes = {}
    # all psh commands are hardlinks
    psh_cmds = psh.get_commands(p)
    for psh_cmd in psh_cmds:
        uptimes[psh_cmd] = psh.uptime(p)
        msg = f"Prompt hasn't been seen after the hardlink touch: /bin/{psh_cmd}"
        psh.assert_cmd(p, f'touch /bin/{psh_cmd}', '', msg)

    assert_timestamp_change(p, psh_cmds, uptimes, '/bin')


@psh.run
def harness(p):
    assert_hardlinks(p)
