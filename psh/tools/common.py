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

import string

from datetime import datetime, timedelta
from typing import Dict

import psh.tools.psh as psh

from psh.tools.psh import CONTROL_CODE, File
from psh.tools.randwrapper import TestRandom

SEPARATOR_PATTERN_BASE = r'(?:' + CONTROL_CODE + r'|\s)'
# acceptable separators: white spaces (wss) + CC, CC + wss, wss
SEPARATOR_PATTERN = SEPARATOR_PATTERN_BASE + r'+?'
# acceptable patterns: white spaces (wss) + CC, CC + wss, wss, nothing
OPTIONAL_SEPARATOR_PATTERN = SEPARATOR_PATTERN_BASE + r'*?'
# all printable chars excluding whitespaces and SH syntax
CHARS = list(set(string.printable) - set(string.whitespace) - set('/<>;|&$!*\\()[]{}\'"`'))


def get_rand_strings(pool, count, random_wrapper: TestRandom, min_chars=8, max_chars=16):
    ''' Returns random names (with length between min_chars and max_chars) from characters pool'''
    # we want to have reproducible test cases
    pool = sorted(pool)
    return [''.join(random_wrapper.choices(pool, k=random_wrapper.randint(min_chars, max_chars))) for _ in range(count)]


def create_testdir(p, dirname):
    # TODO: has to be changed after adding rm implementation and removing test directories
    msg = '\n'.join(['Wrong output when creating a test directory!',
                     'Probably the directory has already been created.',
                     'Try to re-build the project and run specified test second time.'])

    psh.assert_cmd(p, f'mkdir {dirname}', result='success', msg=msg)


def _assert_mtime(target_datetime: datetime, file: File):
    date: datetime = file.datetime
    date_resolution: timedelta = file.datetime_resolution

    # Check if delta is smaller than resolution and allow for file to be up to 2 seconds
    # newer (this can happen because `date` is called first, then `touch`)
    delta = target_datetime - date
    assert delta < date_resolution and delta >= timedelta(seconds=-2), "".join((
        f'The modification time for {file.name} is not equal to the target one! ',
        f'file datetime: {date} target datetime: {target_datetime}'))


def assert_dir_mtimes(p, datetimes: Dict[str, datetime], dir=''):
    ''' Asserts that files (keys in datetimes dictionary) have modification time
    equal to corresponding datetime values with 1 min margin.
    Seconds are not checked. If file's modification year is different from year
    currently set on the device, the hours and minutes will not be checked either. '''

    dir_files = {file.name: file for file in psh.ls(p, dir)}

    for filename, target_datetime in datetimes.items():
        assert filename in dir_files, f'File {filename} is not present in the {dir} directory!'

        _assert_mtime(target_datetime, dir_files[filename])


def assert_file_mtime(p, target_datetime, file_path):
    ''' Asserts that the file has modification time
    equal to corresponding target datetime value with 1 min margin.
    Seconds are not checked. If file's modification year is different from year
    currently set on the device, the hours and minutes will not be checked either. '''
    file = psh.ls(p, file_path)[0]
    _assert_mtime(target_datetime, file)


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
    psh.assert_cmd(p, f'{cmd} {name}', result='success', msg=f'Failed to create: {name} using {cmd}')

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


def _assert_deleted(p, name, dir=False):
    ''' Asserts that the `name` file/directory is properly deleted.
    Name can also be the file/directory absolute path'''

    cmd = 'rmdir' if dir else 'rm'
    psh.assert_cmd(p, f'{cmd} {name}', result='success', msg=f'Failed to delete: {name} using {cmd}')


def assert_file_deleted(p, name):
    _assert_deleted(p, name, dir=False)


def assert_dir_deleted(p, name):
    _assert_deleted(p, name, dir=True)


def assert_deleted_rec(p, dir):
    ''' Asserts that the directory and its contents are deleted recursively '''

    dir_contents = psh.ls(p, dir)
    for file in dir_contents:
        if file.is_dir:
            if file.name in [".", ".."]:
                continue
            assert_deleted_rec(p, dir + "/" + file.name)
        else:
            assert_file_deleted(p, dir + "/" + file.name)
    assert_dir_deleted(p, dir)


def _assert_random(p, pool, path, count, random_wrapper: TestRandom, dir=False):
    ''' Asserts that `count` number of files/directories with random names
    (created by matching chars from the `pool` list) are properly created
    in the `path` directory '''
    psh.assert_cmd(p, f'mkdir {path}', result='success', msg=f'Failed to create {path} directory')
    names = get_rand_strings(pool, count, random_wrapper)

    cmd = 'mkdir' if dir else 'touch'
    for name in names:
        psh.assert_cmd(p, f'{cmd} {path}/{name}', msg=f'Failed to create {name}')

    files = psh.ls(p, path)
    msg = f'The number of created random files/directories is wrong ({len(files)} != {len(names) + 2})'
    # +2 because of . and ..
    assert len(files) == len(names) + 2, msg

    assert_present(name, files, dir=dir)


def assert_random_files(p, pool, path, random_wrapper: TestRandom, count=20):
    _assert_random(p, pool, path, count, random_wrapper, dir=False)


def assert_random_dirs(p, pool, path, random_wrapper: TestRandom, count=20):
    _assert_random(p, pool, path, count, random_wrapper, dir=True)
