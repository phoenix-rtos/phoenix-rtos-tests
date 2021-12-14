# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# common tools for psh related tests
#
# Copyright 2021 Phoenix Systems
# Author: Jakub Sarzyński, Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import random
import string
import psh.tools.psh as psh


CHARS = list(set(string.printable) - set(string.whitespace) - set('/'))


def get_rand_names(pool, count, min_chars=8, max_chars=16):
    ''' Returns random names (with length between min_chars and max_chars) from characters pool'''
    names = [''.join(random.choices(pool, k=random.randint(min_chars, max_chars))) for _ in range(count)]
    return names


def assert_present(name, files, dir=False):
    ''' Asserts that the `name` file/directory is present in the `files` named tuple'''
    for file in files:
        if name == file.name:
            assert file.owner == 'root', f'{name} not owned by root'
            if dir:
                assert file.is_dir, f'{name} is not directory'
            break
    else:
        assert False, f'failed to find {name}'


def assert_created(p, cmd, name):
    ''' Asserts that the `name` file/directory is properly created
    using the apprioprate command touch/mkdir
    name can also be the file/directory absolute path'''
    psh.assert_cmd(p, f'{cmd} {name}', msg=f'Failed to create: {name} using {cmd}')

    path_elements = name.rsplit('/', 1)
    if len(path_elements) == 2:
        name = path_elements[1]
        path = path_elements[0]
    else:
        path = '/'

    files = psh.ls(p, path)
    if cmd == 'mkdir':
        assert_present(name, files, dir=True)
    elif cmd == 'touch':
        assert_present(name, files, dir=False)


def assert_random(p, pool, cmd, path, count=20):
    ''' Asserts that `count` number of files/directories with random names
    (created by matching chars from the `pool` list)
    are properly created in the `path` directory
    using the apprioprate command touch/mkdir'''
    psh.assert_cmd(p, f'mkdir /{path}', msg=f'Failed to create {path} directory')
    names = get_rand_names(pool, count)

    for name in names:
        psh.assert_cmd(p, f'{cmd} /{path}/{name}', msg=f'Failed to create {name}')

    files = psh.ls(p, f'/{path}')
    msg = f'The number of created random files/directories is wrong ({len(files)} != {len(names) + 2})'
    # +2 because of . and ..
    assert len(files) == len(names) + 2, msg

    for name in names:
        if cmd == 'mkdir':
            assert_present(name, files, dir=True)
        elif cmd == 'touch':
            assert_present(name, files, dir=False)
