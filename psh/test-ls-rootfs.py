# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# additional psh ls command test for targets with root file system
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh


def assert_ls_execolor(p):
    p.sendline('ls bin')
    # assumes that some binary is in directory with binaries
    p.expect(psh.esc_codes['set_green'] + 'psh')

    psh.assert_prompt(p, "Prompt hasn't been seen after checking binaries colors")


def assert_ls_pshcmds(p):
    psh_cmds = psh.get_commands(p)

    # there can be other files in list
    other_file_pattern = r'(' + r'(\S+)[ \t]*' + psh.esc_codes["reset_attributes"] + psh.EOL + r'?)*?'
    expected = r'(' + psh.esc_codes['set_green'] + r'('
    for pshcmd in psh_cmds:
        if pshcmd != psh_cmds[0]:
            expected = expected + r'|'
        expected = expected + pshcmd

    expected = expected + r')[ \t]*' + psh.esc_codes["reset_attributes"] + psh.EOL + '?'
    expected = expected + other_file_pattern + r'){' + f'{len(psh_cmds)}' + r'}'

    psh.assert_cmd(p, 'ls bin', expected, '', is_regex=True)


def assert_ls_t(p):
    # newly created directories have 'Jan 1 00:00' creation, so test_ls_dir is older than bin directory
    expected = r'.*?bin.*?test_ls_dir.*?'
    expected = expected + psh.EOL

    msg = "`bin` directory isn't printed before newly created `test_ls_dir` when calling `ls -t`"
    psh.assert_cmd(p, 'ls -t', expected, msg, is_regex=True)


def harness(p):
    psh.init()

    assert_ls_t(p)
    assert_ls_execolor(p)
    assert_ls_pshcmds(p)
