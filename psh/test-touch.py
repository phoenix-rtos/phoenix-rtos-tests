# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh touch command test
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh
import trunner.ctx as ctx
from psh.tools.randwrapper import TestRandom
from psh.tools.common import (CHARS, assert_present, assert_file_created, assert_dir_created,
                              assert_random_files, get_rand_strings, create_testdir,
                              assert_dir_mtimes, assert_file_mtime, assert_deleted_rec)


ROOT_TEST_DIR = 'test_touch_dir'


def assert_executable(p):
    # psh is the example of executable which exists in file system
    msg = "Prompt hasn't been seen after the executable touch: /usr/bin/hello"
    date = psh.date(p)
    if not ctx.target.rootfs:
        psh.assert_cmd(p, 'touch /syspage/psh', result='success', msg=msg)
        assert_dir_mtimes(p, {'psh': date}, '/syspage')
    else:
        psh.assert_cmd(p, 'touch /bin/psh', result='success', msg=msg)
        assert_dir_mtimes(p, {'psh': date}, '/bin')


def assert_devices(p):
    dev_files = psh.ls(p, '/dev')

    # /dev may include directories - skip them
    dev_files = [file for file in psh.ls(p, '/dev') if not file.is_dir]

    # check only 5 devices to save time
    for dev in dev_files[:5]:
        file_path = f'/dev/{dev.name}'
        date = psh.date(p)
        msg = f"Prompt hasn't been seen after the device touch: {file_path}"
        psh.assert_cmd(p, f'touch {file_path}', result='success', msg=msg)
        assert_file_mtime(p, date, file_path)


def assert_multi_arg(p, path, random_wrapper: TestRandom):
    psh.assert_cmd(p, f'mkdir /{path}', result='success', msg=f'Failed to create {path} directory')
    names = get_rand_strings(CHARS, 3, random_wrapper, min_chars=4, max_chars=8)
    args = f'/{path}/{names[0]} ' + f'/{path}/{names[1]} ' + f'/{path}/{names[2]}'
    psh.assert_cmd(p, f'touch {args}', result='success', msg=f'Failed to create {args}')

    files = psh.ls(p, f'/{path}')
    for name in names:
        assert_present(name, files, dir=False)


def assert_file_slash(p):
    """ in psh touch we do not expect error when touching file/ """
    file_path = f'{ROOT_TEST_DIR}/slash_file/'
    assert_file_created(p, file_path)
    psh.assert_cmd(p, f'touch {file_path}', result='success')


def assert_created_dir(p):
    dir_path = f'{ROOT_TEST_DIR}/test_dir'
    assert_dir_created(p, dir_path)
    # touch on already created dir, without checking timestamp, just expecting no output
    psh.assert_cmd(p, f'touch {dir_path}', result='success')
    psh.assert_cmd(p, f'touch {dir_path}/', result='success')


def assert_existing_dirs(p):
    # we assume that syspage directory exists on syspage targets and bin directory exists on root-fs targets
    msg = "Prompt hasn't been seen after the executable touch: /usr/bin/hello"
    date = psh.date(p)
    if not ctx.target.rootfs:
        psh.assert_cmd(p, 'touch /syspage/', result='success', msg=msg)
        assert_dir_mtimes(p, {'syspage': date}, '/')
    else:
        psh.assert_cmd(p, 'touch /bin/', result='success', msg=msg)
        assert_dir_mtimes(p, {'bin': date}, '/')


@psh.run
def harness(p):
    create_testdir(p, ROOT_TEST_DIR)
    random_wrapper = TestRandom(seed=1)

    assert_file_created(p, f'{ROOT_TEST_DIR}/test_file')
    # double touch the same file without checking timestamp
    assert_file_created(p, f'{ROOT_TEST_DIR}/test_file')
    # there are some targets, where max fname length equals 64
    assert_file_created(p, f'{ROOT_TEST_DIR}/' + ''.join(CHARS[:50]))
    assert_file_created(p, f'{ROOT_TEST_DIR}/' + ''.join(CHARS[50:]))
    assert_random_files(p, CHARS, f'{ROOT_TEST_DIR}/random/', random_wrapper, count=20)
    for i in range(10):
        assert_multi_arg(p, f'{ROOT_TEST_DIR}/multi_arg{i}', random_wrapper)
    assert_file_slash(p)

    assert_created_dir(p)
    assert_existing_dirs(p)

    assert_devices(p)
    assert_executable(p)

    assert_deleted_rec(p, ROOT_TEST_DIR)
