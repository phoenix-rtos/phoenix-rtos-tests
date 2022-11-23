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
import trunner.config as config
from psh.tools.common import (CHARS, assert_present, assert_file_created, assert_dir_created,
                              assert_random_files, get_rand_strings, create_testdir, assert_mtime)

ROOT_TEST_DIR = 'test_touch_dir'


def assert_executable(p):
    # psh is the example of executable which exists in file system
    msg = "Prompt hasn't been seen after the executable touch: /usr/bin/hello"
    date = psh.date(p)
    if config.CURRENT_TARGET in config.SYSEXEC_TARGETS:
        psh.assert_cmd(p, 'touch /syspage/psh', msg=msg)
        assert_mtime(p, {'psh': date}, '/syspage')
    else:
        psh.assert_cmd(p, 'touch /bin/psh', msg=msg)
        assert_mtime(p, {'psh': date}, '/bin')


def assert_devices(p):
    dates = {}
    devices = psh.ls_simple(p, '/dev')
    for dev in devices:
        dates[dev] = psh.date(p)
        msg = f"Prompt hasn't been seen after the device touch: /dev/{dev}"
        psh.assert_cmd(p, f'touch /dev/{dev}', msg=msg)

    assert_mtime(p, dates, dir='/dev')


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
    date = psh.date(p)
    if config.CURRENT_TARGET in config.SYSEXEC_TARGETS:
        psh.assert_cmd(p, 'touch /syspage/', msg=msg)
        assert_mtime(p, {'syspage': date}, '/')
    else:
        psh.assert_cmd(p, 'touch /bin/', msg=msg)
        assert_mtime(p, {'bin': date}, '/')


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

    assert_devices(p)
    assert_executable(p)
