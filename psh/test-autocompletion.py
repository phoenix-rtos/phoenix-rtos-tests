# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh sutocompletion and hints test
#
# Copyright 2021 Phoenix Systems
# Author: Mateusz Niewiadomski
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#
import psh.tools.psh as psh
from psh.tools.common import create_testdir, assert_file_created, assert_dir_created, assert_deleted_rec

TAB = '\x09'
ENTER = '\x0D'
ROOT_TEST_DIR = 'test_autocompletion_dir'
TEST_DIR_BASIC = f'{ROOT_TEST_DIR}/loremipsum'


def assert_completion(p, comptab):
    for prefix, suffix in comptab:
        p.send(prefix)
        p.expect_exact(prefix)
        p.send(TAB)
        p.expect_exact(suffix)
    p.send(ENTER)
    psh.assert_prompt(p, msg='No prompt after completion test', timeout=1)


def assert_hints(p, path, hints):
    p.send(path)
    p.expect_exact(path)
    p.send(TAB)
    for h in hints:
        p.expect_exact(h)
    psh.assert_prompt(p, msg='No prompt after printing hints', timeout=1)
    p.send(ENTER)
    psh.assert_prompt(p, msg=f'No prompt after sending the following command: {path}', timeout=1)


@psh.run
def harness(p):
    p.sendline('mkdir etc')
    p.expect_exact('mkdir etc')

    # Create test environment
    create_testdir(p, ROOT_TEST_DIR)
    assert_dir_created(p, f'{ROOT_TEST_DIR}/ipsum')
    assert_dir_created(p, f'{ROOT_TEST_DIR}/lorem')
    assert_dir_created(p, TEST_DIR_BASIC)
    assert_file_created(p, f'{TEST_DIR_BASIC}/dolor.txt')
    assert_file_created(p, f'{TEST_DIR_BASIC}/sit.jpg')
    assert_file_created(p, f'{TEST_DIR_BASIC}/amet.exe')

    # Check autocompletion in command
    assert_completion(p, [['ls e', 'tc/']])
    assert_completion(p, [['ls /e', 'tc/']])
    assert_completion(p, [['ls /etc', '/']])

    assert_completion(p, [['ls test_autocompletion_', 'dir/']])
    assert_completion(p, [['ls /test_autocompletion_', 'dir/']])
    assert_completion(p, [[f'ls {ROOT_TEST_DIR}/i', 'psum/']])

    longpath = f'ls {ROOT_TEST_DIR}/lorem/./../lorem/../../{ROOT_TEST_DIR}/ip'
    assert_completion(p, [[longpath, 'sum']])
    assert_completion(p, [[f'ls ../{ROOT_TEST_DIR}/ip', 'sum']])

    # Check multiple autocompletion in one
    assert_completion(p, [['ls test_autocompletion_', 'dir/'], ['loremi', 'psum/'], ['dol', 'or.txt']])

    # Check hints
    assert_hints(p, path=f'ls {ROOT_TEST_DIR}/', hints=['ipsum', 'lorem', 'loremipsum'])
    assert_hints(p, path=f'ls {ROOT_TEST_DIR}/lorem/../', hints=['ipsum', 'lorem', 'loremipsum'])
    assert_hints(p, path=f'ls {ROOT_TEST_DIR}/loremipsum/', hints=['amet', 'dolor', 'sit'])

    # Cleanup
    assert_deleted_rec(p, ROOT_TEST_DIR)

    # TODO Tests: backspace handling, symlink handling
