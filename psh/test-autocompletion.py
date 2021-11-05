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
from psh.tools.basic import run_psh, assert_only_prompt, assert_prompt


TAB = '\x09'
ENTER = '\x0D'


def mkdir(p, path):
    p.send('mkdir ' + path + ENTER)
    p.expect_exact('mkdir ' + path)
    assert_prompt(p, msg='Cannot make directory: ' + path, timeout=1)


def touch(p, path):
    p.send('touch ' + path + ENTER)
    p.expect_exact('touch ' + path)
    assert_prompt(p, msg='Cannot make file: ' + path, timeout=1)


def assert_completion(p, comptab):
    for prefix, suffix in comptab:
        p.send(prefix)
        p.expect_exact(prefix)
        p.send(TAB)
        p.expect_exact(suffix)
    p.send(ENTER)
    assert_prompt(p, 'No prompt after completion test', timeout=1)


def assert_hints(p, path, hints):
    p.send(path)
    p.expect_exact(path)
    p.send(TAB)
    for h in hints:
        p.expect_exact(h)
    p.send(ENTER)
    assert_prompt(p, 'No prompt after hints test', timeout=1)


def harness(p):
    run_psh(p)
    assert_only_prompt(p)

    mkdir(p, 'etc')

    # Create test environment
    mkdir(p, 'testenv')
    mkdir(p, 'testenv/ipsum')
    mkdir(p, 'testenv/lorem')
    mkdir(p, 'testenv/loremipsum')
    touch(p, 'testenv/loremipsum/dolor.txt')
    touch(p, 'testenv/loremipsum/sit.jpg')
    touch(p, 'testenv/loremipsum/amet.exe')

    # Check autocompletion in command
    assert_completion(p, [['ls e', 'tc/']])
    assert_completion(p, [['ls /e', 'tc/']])
    assert_completion(p, [['ls /etc', '/']])

    assert_completion(p, [['ls testen', 'v/']])
    assert_completion(p, [['ls /testen', 'v/']])
    assert_completion(p, [['ls testenv/i', 'psum/']])

    longpath = 'ls testenv/lorem/./../lorem/../../testenv/ip'
    assert_completion(p, [[longpath, 'sum']])
    assert_completion(p, [['ls ../testenv/ip', 'sum']])

    # Check multiple autocompletion in one
    assert_completion(p, [['ls testen', 'v/'], ['loremi', 'psum/'], ['dol', 'or.txt']])

    # Check hints
    assert_hints(p, path='ls testenv/', hints=['ipsum', 'lorem', 'loremipsum'])
    assert_hints(p, path='ls testenv/lorem/../', hints=['ipsum', 'lorem', 'loremipsum'])
    assert_hints(p, path='ls testenv/loremipsum/', hints=['amet', 'dolor', 'sit'])

    # TODO Tests: backspace handling, symlink handling
    # TODO cleanup folders after tests
