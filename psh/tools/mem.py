# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# common tools for psh mem command tests
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import pexpect
import psh.tools.psh as psh

from collections import namedtuple


def get_meminfo(p):
    Meminfo = namedtuple('Meminfo', ['used_mem', 'boot_mem', 'total_mem', 'used_entries', 'total_entries'])
    p.sendline('mem')
    idx = p.expect_exact(['mem', pexpect.TIMEOUT, pexpect.EOF])
    assert idx == 0, "mem command hasn't been sent properly"

    idx = p.expect([r'\((\d+)\+(\d+)\)/(\d+)\w?B\s+(\d+)/(\d+)\s+entries', pexpect.TIMEOUT, pexpect.EOF])

    assert idx == 0, 'Wrong format of mem command output'
    output = p.match.groups()
    try:
        used_mem, boot_mem, total_mem, used_entries, total_entries = output
    except ValueError:
        assert False, f'wrong mem output: {output}'

    psh.assert_prompt(p, msg='Prompt not seen after sending mem command')
    meminfo = Meminfo(int(used_mem), int(boot_mem), int(total_mem), int(used_entries), int(total_entries))

    return meminfo


def assert_m_arg(p):
    msg = 'Wrong format of mem command output, when calling with  `m` argument'
    exp_regex = r'SEGMENT(\s)+PROT(\s)+FLAGS(\s)+OFFSET(\s)+OBJECT(\r+\n)' + \
                r'(([ \S]*?)(\r+\n))+'

    psh.assert_cmd(p, 'mem -m', exp_regex, msg, is_regex=True)

    msg = 'Wrong format of mem command output, when calling `mem -m kernel`'
    psh.assert_cmd(p, 'mem -m kernel', exp_regex, msg, is_regex=True)

    msg = 'No proper error message seen, when passing wrong argument for mem -m'
    psh.assert_cmd(p, 'mem -m wrong_pid', "mem: could not parse process id: 'wrong_pid'", msg)


def assert_basics(meminfo):
    msg = 'The amount of allocated memory is higher than its total amount'
    assert (meminfo.used_mem + meminfo.boot_mem) <= meminfo.total_mem, msg
    msg = 'The amount of used entries is higher than total amount of entries'
    assert meminfo.used_entries <= meminfo.total_entries, msg


def assert_changes(meminfo, prev_meminfo, more=True):
    msg = 'The amount of boot firmware memory has been changed, when creating a new process'
    assert meminfo.boot_mem == prev_meminfo.boot_mem, msg
    msg = 'The total amount of memory has been changed, when creating a new process'
    assert meminfo.total_mem == prev_meminfo.total_mem, msg
    msg = 'The total amount of entries has been changed, when creating a new process'
    assert meminfo.total_mem == prev_meminfo.total_mem, msg

    if more:
        msg = "The amount of used memory hasn't been increased, when creating a new process"
        assert meminfo.used_mem > prev_meminfo.used_mem, msg
        msg = "The amount of used entries hasn't been increased, when creating a new process"
        assert meminfo.used_entries > prev_meminfo.used_entries, msg
    else:
        msg = "The amount of used memory hasn't been decreased, when killing a process"
        assert meminfo.used_mem < prev_meminfo.used_mem, msg
        msg = "The amount of used entries hasn't been decreased, when killing a process"
        assert meminfo.used_entries < prev_meminfo.used_entries, msg
