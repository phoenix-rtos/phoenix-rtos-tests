# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh mem command test for targets without mmu
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh

from psh.tools.common import get_sleepy_psh_pid
from psh.tools.mem import assert_basics, assert_m_arg, assert_changes, get_meminfo


def assert_format(p):
    msg = 'Wrong format of mem command output, when calling without arguments'
    psh.assert_cmd(p, 'mem', r'(\(\d+\+\d+\)/\d+\w?B)\s+(\d+/\d+\s+entries)(\r+)\n', msg, is_regex=True)


def harness(p):
    psh.init(p)

    assert_format(p)
    assert_m_arg(p)
    psh.assert_cmd(p, 'mem -p', 'mem: Page view unavailable')

    prev_meminfo = get_meminfo(p)
    assert_basics(prev_meminfo)
    psh.assert_exec(p, 'psh', '', 'Prompt not seen after creating new psh process')

    meminfo = get_meminfo(p)
    assert_changes(meminfo, prev_meminfo, more=True)

    prev_meminfo = meminfo
    sleepy_psh_pid = get_sleepy_psh_pid(p)
    assert sleepy_psh_pid != -1, 'Getting pid of sleepy psh process failed'
    psh.assert_cmd(p, f'kill {sleepy_psh_pid}', '', 'Prompt not seen after killing sleepy psh process')
    meminfo = get_meminfo(p)
    assert_changes(meminfo, prev_meminfo, more=False)
