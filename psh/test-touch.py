# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh touch command text
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

from psh.tools.common import CHARS, assert_present, assert_created, assert_random, get_rand_names
import psh.tools.psh as psh


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
    # hello is the example of executable which exists in file system
    # if the executable is not present, the warning will be printed
    usrbin_content = psh.ls_simple(p, '/usr/bin')
    if 'hello' in usrbin_content:
        msg = "Prompt hasn't been seen after the executable touch: /usr/bin/hello"
        uptime = psh.uptime(p)
        psh.assert_cmd(p, 'touch /usr/bin/hello', '', msg)
        assert_timestamp_change(p, ['hello'], {'hello': uptime}, '/usr/bin')
    else:
        print('\nWarning: touching executables not tested, no hello binary in /usr/bin\n')


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
    names = get_rand_names(CHARS, 3, min_chars=4, max_chars=8)
    args = f'/{path}/{names[0]} ' + f'/{path}/{names[1]} ' + f'/{path}/{names[2]}'
    psh.assert_cmd(p, f'touch {args}', msg=f'Failed to create {args}')

    files = psh.ls(p, f'/{path}')
    for name in names:
        assert_present(name, files, dir=False)


def harness(p):
    testdir_name = 'test_touch_dir'
    psh.init(p)

    assert_created(p, 'mkdir', testdir_name)
    assert_created(p, 'touch', f'{testdir_name}/test_file')
    # double touch the same file without checking timestamp
    assert_created(p, 'touch', f'{testdir_name}/test_file')
    assert_created(p, 'touch', f'{testdir_name}/' + ''.join(CHARS))
    assert_random(p, CHARS, 'touch', f'{testdir_name}/random', count=20)
    for i in range(10):
        assert_multi_arg(p, f'{testdir_name}/multi_arg{i}')

    assert_executable(p)
    assert_symlinks(p)
    assert_devices(p)
