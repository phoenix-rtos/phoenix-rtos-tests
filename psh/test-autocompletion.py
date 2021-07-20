# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# "auth" psh applet test
#
# Copyright 2021 Phoenix Systems
# Author: Mateusz Niewiadomski
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#
import re

from psh.tools.basic import run_psh, assert_only_prompt, assert_prompt


TAB = '\t'
ENTER = '\n'


def mkdir(p, path):
    p.send('mkdir ' + path + ENTER)
    p.expect_exact('mkdir ' + path)
    assert_prompt(p, msg='Cannot make directory: ' + path)


def touch(p, path):
    p.send('touch ' + path + ENTER)
    p.expect_exact('touch ' + path)
    assert_prompt(p, msg='Cannot make file: ' + path)


def assert_completion(p, incomplete, complete):
    p.send(incomplete)
    p.expect_exact(incomplete)
    p.send(TAB)
    if complete.startswith(incomplete):
        addition = re.sub('^' + incomplete, '', complete)
        p.expect_exact(addition)
        p.send(ENTER)
        assert_prompt(p)
    else:
        raise Exception("Error in test: uncomplete path not subset of complete path")


def assert_multiple_completion(p, comptab):
    for pair in comptab:
        p.send(pair[0])
        p.expect_exact(pair[0])
        p.send(TAB)
        p.expect_exact(pair[1])
    p.send(ENTER)
    assert_prompt(p)


def assert_hints(p, path, hints):
    p.send(path)
    p.expect_exact(path)
    p.send(TAB)
    for h in hints:
        p.expect_exact(h)
    p.send(ENTER)
    assert_prompt(p)


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

    # Check autocompletion
    assert_completion(p, 'e', 'etc/')
    assert_completion(p, '/e', '/etc/')
    assert_completion(p, '/etc', '/etc/')

    assert_completion(p, 'testen', 'testenv/')
    assert_completion(p, '/testen', '/testenv/')
    assert_completion(p, 'testenv/i', 'testenv/ipsum/')

    longpath = 'ls testenv/lorem/./../lorem/../../testenv/ip'
    assert_completion(p, longpath, longpath + 'sum')
    assert_completion(p, '../testenv/ip', '../testenv/ipsum')

    # Check autocompletion in command
    assert_completion(p, 'ls e', 'ls etc/')
    assert_completion(p, 'ls /e', 'ls /etc/')
    assert_completion(p, 'ls /etc', 'ls /etc/')

    assert_completion(p, 'ls testen', 'ls testenv/')
    assert_completion(p, 'ls /testen', 'ls /testenv/')
    assert_completion(p, 'ls testenv/i', 'ls testenv/ipsum/')

    assert_completion(p, longpath, longpath + 'sum')
    assert_completion(p, 'ls ../testenv/ip', 'ls ../testenv/ipsum')

    # Check multiple autocompletion
    assert_multiple_completion(p, [['testen', 'v/'], ['loremi', 'psum/'], ['dol', 'or.txt']])

    # Check hints
    assert_hints(p, path='testenv/', hints=['ipsum', 'lorem', 'loremipsum'])
    assert_hints(p, path='testenv/lorem/../', hints=['ipsum', 'lorem', 'loremipsum'])
    assert_hints(p, path='testenv/loremipsum/', hints=['amet', 'dolor', 'sit'])
