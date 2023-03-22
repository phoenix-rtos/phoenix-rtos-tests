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

import psh.tools.psh as psh
from psh.tools.psh import EOT
from psh.tools.randwrapper import TestRandom
from psh.tools.common import CHARS, get_rand_strings

ROOT_TEST_DIR = 'test_runfile_dir'


def get_root_dirs(p):
    root_dirs = []
    root_content = psh.ls(p)
    for root_file in root_content:
        if root_file.is_dir:
            root_dirs.append(root_file.name)
    return root_dirs


def assert_nonexistent(p, root_dirs, random_wrapper: TestRandom):
    psh.assert_prompt_after_cmd(p, '/nonexistent_file', result='fail')

    rfnames = get_rand_strings(CHARS, 10, random_wrapper)
    for rfname in rfnames:
        psh.assert_prompt_after_cmd(p, f'/{rfname}', result='fail')

        for root_dir in root_dirs:
            psh.assert_prompt_after_cmd(p, f'/{root_dir}/{rfname}', result='fail')


def assert_dirs(p, root_dirs):
    psh.assert_prompt_after_cmd(p, f'/{ROOT_TEST_DIR}', result='fail')

    for root_dir in root_dirs:
        psh.assert_prompt_after_cmd(p, f'/{root_dir}', result='fail')


def assert_hardlinks(p):
    # psh applets are examples of hardlinks that exists in fs.
    # When busybox port is used their applets overwrite psh hardlinks with same names.
    # That's why we test only one phoenix-specific psh applet
    # Just checking whether executing runfile on them prints expected format for this cmd
    msg = 'Wrong format of mem command output, when calling without arguments'
    psh.assert_cmd(p,
                   '/bin/mem',
                   expected=r'(\(\d+\+\d+\)/\d+\w?B)\s+(\d+/\d+\s+entries)(\r+)\n',
                   result='success',
                   msg=msg,
                   is_regex=True)


def _exit_spawned_psh(p):
    p.send(EOT)
    p.expect(r'exit(\r+)\n')


def assert_executables(p, ctx):
    # psh is the example of executable which exists in file system
    if ctx.target.rootfs:
        psh.assert_cmd(p, '/bin/psh', result='success')
    else:
        psh.assert_cmd(p, '/syspage/psh', result='success')

    _exit_spawned_psh(p)


def assert_devices(p):
    devs = psh.ls_simple(p, '/dev')
    for dev in devs:
        psh.assert_prompt_after_cmd(p, f'/dev/{dev}', result='fail')


def assert_text_files(p):
    etc_content = psh.ls_simple(p, '/etc')
    if 'passwd' in etc_content:
        psh.assert_cmd(p, '/etc/passwd', result='fail')

    psh.assert_cmd(p, 'touch testfile', result='success')
    psh.assert_cmd(p, '/testfile', result='fail')


@psh.run
def harness(p, ctx):
    random_wrapper = TestRandom(seed=1)
    root_dirs = get_root_dirs(p)
    assert_nonexistent(p, root_dirs, random_wrapper)
    assert_dirs(p, root_dirs)
    assert_hardlinks(p)
    assert_executables(p, ctx)
    # skipped test case because of https://github.com/phoenix-rtos/phoenix-rtos-project/issues/262
    # assert_devices(p)
    assert_text_files(p)
