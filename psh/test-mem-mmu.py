# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh mem command test for mmu targets
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#


import pexpect
import psh.tools.psh as psh

from psh.tools.common import get_sleepy_psh_pid
from psh.tools.mem import assert_basics, assert_m_arg, assert_changes, get_meminfo


def count_app_pages(p):
    p.sendline('mem -p')
    p.expect(r'mem -p(\r+)\n')

    idx = p.expect([r'(.+?)(\r)+\n.*\(psh\)% ', pexpect.TIMEOUT, pexpect.EOF])
    assert idx == 0, "Prompt not seen after calling `mem -p`"
    line = p.match.group(1)
    app_count = line.count('A')

    return app_count


def assert_format(p):
    msg = 'Wrong format of mem command output, when calling without arguments'
    psh.assert_cmd(p, 'mem', r'(\(\d+\+\d+\)/\d+\w?B)\s+(\d+/\d+\s+entries)(\r+)\n', msg, is_regex=True)
    msg = 'Wrong format of mem command output, when calling with  `p` argument'
    psh.assert_cmd(p, 'mem -p', r'[ABCHKPSUYx\.\d\[\]]+?(\r+)\n', msg, is_regex=True)


def harness(p):
    psh.init(p)

    assert_format(p)
    assert_m_arg(p)

    prev_appages_count = count_app_pages(p)
    prev_meminfo = get_meminfo(p)
    assert_basics(prev_meminfo)
    psh.assert_exec(p, 'psh', '', 'Prompt not seen after creating new psh process')

    meminfo = get_meminfo(p)
    assert_changes(meminfo, prev_meminfo, more=True)
    appages_count = count_app_pages(p)
    msg = "Count of app pages hasn't been increased, when creating new psh process"
    assert appages_count > prev_appages_count, msg

    prev_meminfo = meminfo
    prev_appages_count = appages_count
    sleepy_psh_pid = get_sleepy_psh_pid(p)
    assert sleepy_psh_pid != -1, 'Getting pid of sleepy psh process failed'
    psh.assert_cmd(p, f'kill {sleepy_psh_pid}', '', 'Prompt not seen after killing sleepy psh process')
    meminfo = get_meminfo(p)
    assert_changes(meminfo, prev_meminfo, more=False)
    msg = "Count of app pages hasn't been decreased, when killing sleepy psh process"
    assert count_app_pages(p) < prev_appages_count, msg
