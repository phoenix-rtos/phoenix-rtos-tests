# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# common tools for psh related tests
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import random
import string

import psh.tools.psh as psh

from psh.tools.psh import CONTROL_CODE

# acceptable separators: white spaces (wss) + CC, CC + wss, wss
SEPARATOR_PATTERN = r'(?:' + CONTROL_CODE + r'|\s)+'
CHARS = list(set(string.printable) - set(string.whitespace) - set('/'))


def get_rand_strings(pool, count, min_chars=8, max_chars=16):
    ''' Returns random names (with length between min_chars and max_chars) from characters pool'''
    return [''.join(random.choices(pool, k=random.randint(min_chars, max_chars))) for _ in range(count)]


def create_testdir(p, dirname):
    # TODO: has to be changed after adding rm implementation and removing test directories
    msg = '\n'.join(['Wrong output when creating a test directory!',
                     'Probably the directory has already been created.',
                     'Try to re-build the project and run specified test second time.'])

    psh.assert_cmd(p, f'mkdir {dirname}', '', msg)


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


def assert_present(name, files, dir=False):
    ''' Asserts that the `name` file/directory is present in the `files` named tuple'''
    for file in files:
        if name == file.name:
            # TODO: enable after adding multiple user support
            # assert file.owner == 'root', f'{name} not owned by root'
            if dir:
                assert file.is_dir, f'{name} is not directory'
            break
    else:
        assert False, f'failed to find {name}'


def _assert_created(p, name, dir=False):
    ''' Asserts that the `name` file/directory is properly created
    name can also be the file/directory absolute path'''

    cmd = 'mkdir' if dir else 'touch'
    psh.assert_cmd(p, f'{cmd} {name}', msg=f'Failed to create: {name} using {cmd}')

    name = name[:-1] if name.endswith('/') else name

    path_elements = name.rsplit('/', 1)
    if len(path_elements) == 2:
        name = path_elements[1]
        path = path_elements[0]
    else:
        path = '/'

    files = psh.ls(p, path)
    assert_present(name, files, dir=dir)


def assert_file_created(p, name):
    _assert_created(p, name, dir=False)


def assert_dir_created(p, name):
    _assert_created(p, name, dir=True)


def _assert_random(p, pool, path, count, dir=False):
    ''' Asserts that `count` number of files/directories with random names
    (created by matching chars from the `pool` list) are properly created
    in the `path` directory '''
    psh.assert_cmd(p, f'mkdir {path}', msg=f'Failed to create {path} directory')
    names = get_rand_strings(pool, count)

    cmd = 'mkdir' if dir else 'touch'
    for name in names:
        psh.assert_cmd(p, f'{cmd} {path}/{name}', msg=f'Failed to create {name}')

    files = psh.ls(p, path)
    msg = f'The number of created random files/directories is wrong ({len(files)} != {len(names) + 2})'
    # +2 because of . and ..
    assert len(files) == len(names) + 2, msg

    assert_present(name, files, dir=dir)


def assert_random_files(p, pool, path, count=20):
    _assert_random(p, pool, path, count, dir=False)


def assert_random_dirs(p, pool, path, count=20):
    _assert_random(p, pool, path, count, dir=True)
