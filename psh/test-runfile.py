# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh runfile applet test
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
from psh.tools.common import CHARS, get_rand_names


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

    rfnames = get_rand_names(CHARS, 10)
    for rfname in rfnames:
        cmd = '/nonexistent_file'
        msg = f'Wrong error message, when executing: /{rfname} {rfname} {rfname}'
        psh.assert_cmd(p, f'/{rfname}', f'psh: /{rfname} not found', msg)
        for root_dir in root_dirs:
            msg = f'Wrong error message, when executing: /{root_dir}/{rfname}'
            psh.assert_cmd(p, f'/{root_dir}/{rfname}', f'psh: /{root_dir}/{rfname} not found', msg)


def assert_dirs(p, root_dirs):
    dirname = 'test_runfile_dir'
    msg = f'Creating the following directory failed: {dirname}'
    psh.assert_cmd(p, f'mkdir {dirname}', '', msg)
    psh.assert_cmd(p, f'/{dirname}', f'psh: /{dirname} is not an executable')

    for root_dir in root_dirs:
        msg = f'Wrong error message, when executing: /{root_dir}'
        psh.assert_cmd(p, f'/{root_dir}', f'psh: /{root_dir} is not an executable', msg)


def assert_symlinks(p):
    p.sendline('/bin/ls')
    psh.assert_prompt(p, "Prompt hasn't been seen after executing: /bin/ls")

    help_cmds = psh.get_commands(p)
    p.sendline('/bin/help')
    for help_cmd in help_cmds:
        idx = p.expect_exact([help_cmd, pexpect.TIMEOUT, pexpect.EOF])
        assert idx == 0, f"Help output, when executing by runfile doesn't print the following command: {help_cmd}"
    psh.assert_prompt(p, "Prompt hasn't been seen after executing: /bin/help")


def assert_text_files(p):
    msg = 'Wrong output when executing text file'
    psh.assert_cmd(p, '/etc/passwd', '', msg)

    msg = 'Creating testfile by touch command failed'
    psh.assert_cmd(p, 'touch testfile', '', msg)

    msg = 'Wrong output when executing empty file'
    psh.assert_cmd(p, '/testfile', '', msg)


def assert_executables(p):
    # hello is the example of executable which exists in file system
    # if the executable is not present, the warning will be printed
    usrbin_content = psh.ls_simple(p, '/usr/bin')
    if 'hello' in usrbin_content:
        msg = "Wrong output from /usr/bin/hello program, when executing by runfile"
        psh.assert_cmd(p, '/usr/bin/hello', 'Hello World!!', msg)
        # passing multiple number of arguments should have the same effect
        psh.assert_cmd(p, '/usr/bin/hello arg1 arg2 arg3 arg4 arg5', 'Hello World!!', msg)
    else:
        print('\nWarning: executing binaries by runfile not tested, no hello binary in /usr/bin\n')


def assert_devices(p):
    devs = psh.ls_simple(p, '/dev')
    for dev in devs:
        msg = f'Wrong error message after executing the following device by runfile: {dev}'
        psh.assert_cmd(p, f'/dev/{dev}', f'psh: /dev/{dev} not found', msg)
        p.sendline('ls')
        psh.assert_prompt(p, "Prompt hasn't been seen when running ls after executing devices by runfile")


def harness(p):
    psh.init(p)

    root_dirs = get_root_dirs(p)
    assert_nonexistent(p, root_dirs)
    assert_dirs(p, root_dirs)
    assert_symlinks(p)
    # skipped test case because of https://github.com/phoenix-rtos/phoenix-rtos-project/issues/262
    # assert_devices(p)
    assert_text_files(p)
