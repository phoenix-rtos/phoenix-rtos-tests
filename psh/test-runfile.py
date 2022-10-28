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


def get_root_dirs(p):
    root_dirs = []
    root_content = psh.ls(p)
    for root_file in root_content:
        if root_file.is_dir:
            root_dirs.append(root_file.name)
    return root_dirs


def assert_nonexistent(p, root_dirs):
    cmd = '/nonexistent_file'
    msg = 'Wrong error message, when executing nonexistent file'
    psh.assert_cmd(p, cmd, f'psh: {cmd} not found', msg)

    rfnames = get_rand_strings(CHARS, 10)
    for rfname in rfnames:
        cmd = '/nonexistent_file'
        msg = f'Prompt not seen after executing: /{rfname}'
        psh.assert_prompt_after_cmd(p, f'/{rfname}', msg)
        psh.assert_cmd_failed(p)

        msg = f'Wrong error message, when executing: /{rfname}'
        psh.assert_cmd(p, f'/{rfname}', f'psh: /{rfname} not found', msg)
        for root_dir in root_dirs:
            msg = f'Prompt not seen after executing: /{root_dir}/{rfname}'
            psh.assert_prompt_after_cmd(p, f'/{rfname}', msg)
            psh.assert_cmd_failed(p)


def assert_dirs(p, root_dirs):
    dirname = 'test_runfile_dir'
    msg = f'Creating the following directory failed: {dirname}'
    psh.assert_cmd(p, f'mkdir {dirname}', '', msg)
    psh.assert_cmd_successed(p)

    msg = f'Prompt not seen after executing: /{dirname}'
    psh.assert_prompt_after_cmd(p, f'/{dirname}', msg)
    psh.assert_cmd_failed(p)

    # psh.assert_cmd(p, f'/{dirname}', f'psh: /{dirname} is not an executable')

    for root_dir in root_dirs:
        msg = f'Prompt not seen after executing: /{root_dir}'
        psh.assert_prompt_after_cmd(p, f'/{dirname}', msg)
        psh.assert_cmd_failed(p)


def assert_symlinks(p):
    p.sendline('/bin/ls')
    psh.assert_prompt(p, "Prompt hasn't been seen after executing: /bin/ls")
    psh.assert_cmd_successed(p)

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
    msg = "Prompt hasn't been seen after executing psh using runfile!"
    if config.CURRENT_TARGET in config.SYSEXEC_TARGETS:
        psh.assert_cmd(p, '/syspage/psh', '', msg)
    else:
        psh.assert_cmd(p, '/bin/psh', '', msg)

    psh.assert_cmd_successed(p)
    _exit_spawned_psh(p)


def assert_devices(p):
    devs = psh.ls_simple(p, '/dev')
    for dev in devs:
        msg = f'Prompt not seen after executing: /dev/{dev}'
        psh.assert_prompt_after_cmd(p, f'/dev/{dev}', msg)
        psh.assert_cmd_failed(p)


def assert_text_files(p):
    etc_content = psh.ls_simple(p, '/etc')
    if 'passwd' in etc_content:
        msg = 'Wrong output when executing text file'
        psh.assert_cmd(p, '/etc/passwd', '', msg)
        psh.assert_cmd_failed(p)

    msg = 'Creating testfile by touch command failed'
    psh.assert_cmd(p, 'touch testfile', '', msg)
    psh.assert_cmd_successed(p)

    msg = 'Wrong output when executing empty file'
    psh.assert_cmd(p, '/testfile', '', msg)
    psh.assert_cmd_failed(p)


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
