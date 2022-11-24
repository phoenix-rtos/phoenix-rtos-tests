# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh touch command text
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh
import trunner.config as config
from psh.tools.common import (CHARS, assert_present, assert_file_created, assert_dir_created,
                              assert_random_files, get_rand_strings, create_testdir)

ROOT_TEST_DIR = 'test_touch_dir'


def assert_timestamp_change(p, touched_files, uptimes, dir=''):
    files = psh.ls(p, dir)
    for file in files:
        if file.name in touched_files:
            uptime = uptimes[file.name]
            # uptime has to be got before touch
            # to prevent failed assertion when typing uptime in 00:59 and touch in 01:00
            # 1 min margin is applied
            next_minute = f'{(int(uptime.minute)+1):02d}'
            if file.date.time == f'{uptime.hour}:{next_minute}':
                uptime_str = f'{uptime.hour}:{next_minute}'
            else:
                uptime_str = f'{uptime.hour}:{uptime.minute}'
            # when writing test, system date is always set to Jan 01 00:00
            # so all touched files should have the following timestamp: Jan 01 00:00 + uptime
            msg = f"The file's timestamp hasn't been changed after touch: {dir}/{file.name}, timestamp = {file.date}"
            assert file.date == ('Jan', '1', uptime_str), msg


def assert_symlinks(p):
    uptimes = {}
    # all psh commands are symlinks
    psh_cmds = psh.get_commands(p)
    for psh_cmd in psh_cmds:
        uptimes[psh_cmd] = psh.uptime(p)
        msg = f"Prompt hasn't been seen after the symlink touch: /bin/{psh_cmd}"
        psh.assert_cmd(p, f'touch /bin/{psh_cmd}', '', msg)

    assert_timestamp_change(p, psh_cmds, uptimes, '/bin')


def assert_executable(p):
    # psh is the example of executable which exists in file system
    msg = "Prompt hasn't been seen after the executable touch: /usr/bin/hello"
    uptime = psh.uptime(p)
    if config.CURRENT_TARGET in config.SYSEXEC_TARGETS:
        psh.assert_cmd(p, 'touch /syspage/psh', '', msg)
        assert_timestamp_change(p, ['psh'], {'psh': uptime}, '/syspage')
    else:
        psh.assert_cmd(p, 'touch /bin/psh', '', msg)
        assert_timestamp_change(p, ['psh'], {'psh': uptime}, '/bin')


def assert_devices(p):
    uptimes = {}
    devices = psh.ls_simple(p, '/dev')
    for dev in devices:
        uptimes[dev] = psh.uptime(p)
        msg = f"Prompt hasn't been seen after the device touch: /dev/{dev}"
        psh.assert_cmd(p, f'touch /dev/{dev}', '', msg)

    assert_timestamp_change(p, devices, uptimes, dir='/dev')


def assert_multi_arg(p, path):
    psh.assert_cmd(p, f'mkdir /{path}', msg=f'Failed to create {path} directory')
    names = get_rand_strings(CHARS, 3, min_chars=4, max_chars=8)
    args = f'/{path}/{names[0]} ' + f'/{path}/{names[1]} ' + f'/{path}/{names[2]}'
    psh.assert_cmd(p, f'touch {args}', msg=f'Failed to create {args}')

    files = psh.ls(p, f'/{path}')
    for name in names:
        assert_present(name, files, dir=False)


def assert_file_slash(p):
    """ in psh touch we do not expect error when touching file/ """
    file_path = f'{ROOT_TEST_DIR}/slash_file/'
    assert_file_created(p, file_path)
    psh.assert_cmd(p, f'touch {file_path}')


def assert_created_dir(p):
    dir_path = f'{ROOT_TEST_DIR}/test_dir'
    assert_dir_created(p, dir_path)
    # touch on already created dir, without checking timestamp, just expecting no output
    psh.assert_cmd(p, f'touch {dir_path}')
    psh.assert_cmd(p, f'touch {dir_path}/')


def assert_existing_dirs(p):
    # we assume that syspage directory exists on syspage targets and bin directory exists on root-fs targets
    msg = "Prompt hasn't been seen after the executable touch: /usr/bin/hello"
    uptime = psh.uptime(p)
    if config.CURRENT_TARGET in config.SYSEXEC_TARGETS:
        psh.assert_cmd(p, 'touch /syspage/', '', msg)
        assert_timestamp_change(p, ['syspage'], {'syspage': uptime}, '/')
    else:
        psh.assert_cmd(p, 'touch /bin/', '', msg)
        assert_timestamp_change(p, ['bin'], {'bin': uptime}, '/')


@psh.run
def harness(p):
    create_testdir(p, ROOT_TEST_DIR)

    assert_file_created(p, f'{ROOT_TEST_DIR}/test_file')
    # double touch the same file without checking timestamp
    assert_file_created(p, f'{ROOT_TEST_DIR}/test_file')
    # there are some targets, where max fname length equals 64
    assert_file_created(p, f'{ROOT_TEST_DIR}/' + ''.join(CHARS[:50]))
    assert_file_created(p, f'{ROOT_TEST_DIR}/' + ''.join(CHARS[50:]))
    assert_random_files(p, CHARS, f'{ROOT_TEST_DIR}/random/', count=20)
    for i in range(10):
        assert_multi_arg(p, f'{ROOT_TEST_DIR}/multi_arg{i}')
    assert_file_slash(p)

    assert_created_dir(p)
    assert_existing_dirs(p)

    assert_symlinks(p)
    assert_devices(p)
    assert_executable(p)