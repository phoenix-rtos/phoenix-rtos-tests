# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh runfile applet test
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
import trunner.config as config
from psh.tools.psh import EOT
from psh.tools.common import CHARS, get_rand_strings

ROOT_TEST_DIR = 'test_runfile_dir'


def get_root_dirs(p):
    root_dirs = []
    root_content = psh.ls(p)
    for root_file in root_content:
        if root_file.is_dir:
            root_dirs.append(root_file.name)
    return root_dirs


def assert_nonexistent(p, root_dirs):
    psh.assert_prompt_after_cmd(p, '/nonexistent_file', result='fail')

    rfnames = get_rand_strings(CHARS, 10)
    for rfname in rfnames:
        psh.assert_prompt_after_cmd(p, f'/{rfname}', result='fail')

        for root_dir in root_dirs:
            psh.assert_prompt_after_cmd(p, f'/{root_dir}/{rfname}', result='fail')


def assert_dirs(p, root_dirs):
    psh.assert_prompt_after_cmd(p, f'/{ROOT_TEST_DIR}', result='fail')

    for root_dir in root_dirs:
        psh.assert_prompt_after_cmd(p, f'/{root_dir}', result='fail')


def assert_symlinks(p):
    psh.assert_prompt_after_cmd(p, '/bin/ls', result='success')

    help_cmds = psh.get_commands(p)
    p.sendline('/bin/help')
    for help_cmd in help_cmds:
        idx = p.expect_exact([help_cmd, pexpect.TIMEOUT, pexpect.EOF])
        assert idx == 0, f"Help output, when executing by runfile doesn't print the following command: {help_cmd}"
    psh.assert_prompt(p, "Prompt hasn't been seen after executing: /bin/help")
    psh.assert_cmd_successed(p)


def _exit_spawned_psh(p):
    p.send(EOT)
    p.expect(r'exit(\r+)\n')


def assert_executables(p):
    # psh is the example of executable which exists in file system
    if config.CURRENT_TARGET in config.SYSEXEC_TARGETS:
        psh.assert_cmd(p, '/syspage/psh', '', result='success')
    else:
        psh.assert_cmd(p, '/bin/psh', '', result='success')

    _exit_spawned_psh(p)


def assert_devices(p):
    devs = psh.ls_simple(p, '/dev')
    for dev in devs:
        psh.assert_prompt_after_cmd(p, f'/dev/{dev}', result='fail')


def assert_text_files(p):
    etc_content = psh.ls_simple(p, '/etc')
    if 'passwd' in etc_content:
        psh.assert_cmd(p, '/etc/passwd', '', result='fail')

    psh.assert_cmd(p, 'touch testfile', '', result='success')
    psh.assert_cmd(p, '/testfile', '', result='fail')


def harness(p):
    psh.init(p)

    root_dirs = get_root_dirs(p)
    assert_nonexistent(p, root_dirs)
    assert_dirs(p, root_dirs)
    assert_symlinks(p)
    assert_executables(p)
    # skipped test case because of https://github.com/phoenix-rtos/phoenix-rtos-project/issues/262
    # assert_devices(p)
    assert_text_files(p)
