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
from psh.tools.common import (CHARS, assert_present, assert_file_created, assert_random_files,
                              get_rand_strings, create_testdir)

ROOT_TEST_DIR = 'test_touch_dir'


def is_hello_present(p):
    psh.assert_prompt_after_cmd(p, 'ls -d /usr/bin')
    ret_code = psh.get_exit_code(p)

    if ret_code != 0:
        return False

    usrbin_content = psh.ls_simple(p, '/usr/bin')
    return True if 'hello' in usrbin_content else False


def assert_timestamp_change(p, touched_files, uptimes, dir=''):
    files = psh.ls(p, dir)
    for file in files:
        if file.name in touched_files:
            uptime = uptimes[file.name]
            msg = f"The file's timestamp hasn't been changed after touch: {dir}/{file.name}, timestamp = {file.date}"
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
            assert file.date == ('Jan', '1', uptime_str), msg


def assert_symlinks(p):
    uptimes = {}
    # all psh commands are symlinks
    psh_cmds = psh.get_commands(p)
    for psh_cmd in psh_cmds:
        msg = f"Prompt hasn't been seen after the symlink touch: /bin/{psh_cmd}"
        uptime = psh.uptime(p)
        psh.assert_cmd(p, f'touch /bin/{psh_cmd}', '', msg)
        uptimes[psh_cmd] = uptime
    assert_timestamp_change(p, psh_cmds, uptimes, '/bin')


def assert_executable(p):
    # hello is the example of executable which likely exists in file system
    msg = "Prompt hasn't been seen after the executable touch: /usr/bin/hello"
    uptime = psh.uptime(p)
    psh.assert_cmd(p, 'touch /usr/bin/hello', '', msg)
    assert_timestamp_change(p, ['hello'], {'hello': uptime}, '/usr/bin')


def assert_devices(p):
    uptimes = {}
    devices = psh.ls_simple(p, '/dev')
    for dev in devices:
        msg = f"Prompt hasn't been seen after the device touch: /dev/{dev}"
        uptime = psh.uptime(p)
        psh.assert_cmd(p, f'touch /dev/{dev}', '', msg)
        uptimes[dev] = uptime
    assert_timestamp_change(p, devices, uptimes, dir='/dev')


def assert_multi_arg(p, path=''):
    psh.assert_cmd(p, f'mkdir /{path}', msg=f'Failed to create {path} directory')
    names = get_rand_strings(CHARS, 3, min_chars=4, max_chars=8)
    args = f'/{path}/{names[0]} ' + f'/{path}/{names[1]} ' + f'/{path}/{names[2]}'
    psh.assert_cmd(p, f'touch {args}', msg=f'Failed to create {args}')

    files = psh.ls(p, f'/{path}')
    for name in names:
        assert_present(name, files, dir=False)


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

    assert_symlinks(p)
    assert_devices(p)

    # This case will only be executed on targets where the hello executable is present
    if is_hello_present:
        assert_executable(p)
