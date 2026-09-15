# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh relaunching test
#
# Copyright 2026 Phoenix Systems
# Author: Adam Greloch
#
# SPDX-License-Identifier: BSD-3-Clause
#

import psh.tools.psh as psh

RELAUNCHES = 3


@psh.run
def harness(p):
    """Exits psh back to the shell that spawned it and launches it anew, several
    times in a row.

    An interactive psh puts itself in its own process group and makes it the
    foreground group of the terminal. If it does not hand the terminal back on
    exit, the terminal keeps naming a process group that no longer exists and
    the next psh waits for that group forever. Shells that run their jobs in
    their own process group (busybox ash built without job control, used on
    rootfs targets) hit this on the very first relaunch, as the group dies
    together with psh."""

    # psh.run() has already launched the psh under test
    for i in range(RELAUNCHES):
        psh.deinit(p)
        psh.init(p)
        psh.assert_cmd(
            p,
            "echo relaunched",
            expected="relaunched",
            msg=f"psh not usable after relaunch no. {i + 1}",
        )
