# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh ls command test
#
# Copyright 2021, 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh
from psh.tools.common import CONTROL_CODE, OPTIONAL_CONTROL_CODE, create_testdir

# acceptable separators: white spaces (wss) + CC, CC + wss, wss
SEPARATOR_PATTERN = r'(?:' + CONTROL_CODE + r'|\s)+'
FNAME_PATTERN = r'(?P<fname>\S+)'

ROOT_TEST_DIR = 'test_ls_dir'
TEST_DIR_BASIC = f'{ROOT_TEST_DIR}/basic'


def prepare_testdir(p, files):
    msg = 'Wrong output when creating basic directory!'
    psh.assert_cmd(p, f'mkdir {TEST_DIR_BASIC}', '', msg)
    cmd = 'touch'
    for file in files:
        if file == 'dir':
            continue
        cmd += f' {TEST_DIR_BASIC}/{file}'

    msg = 'Wrong output when creating basic test files!'
    psh.assert_cmd(p, cmd, '', msg)

    msg = 'Wrong output when creating internal test directory!'
    psh.assert_cmd(p, f'mkdir {TEST_DIR_BASIC}/dir', '', msg)


def create_pattern(files, separate_lines=False):
    newline_separator = OPTIONAL_CONTROL_CODE + r'(\r+\n)'
    pattern = r''
    for file in files:
        pattern += OPTIONAL_CONTROL_CODE + f'{file}'
        if separate_lines:
            pattern += newline_separator
        else:
            pattern += SEPARATOR_PATTERN
    return pattern


def assert_ls_err(p):
    msg = 'Wrong error message, when trying to print nonexistent directory content'
    psh.assert_cmd(p, 'ls nonexistentDir', "ls: can't access nonexistentDir: no such file or directory", msg)


def assert_ls_noarg(p, files):
    expected = create_pattern(sorted(files))
    msg = 'Wrong content of listed directory, when calling ls without arguments'
    psh.assert_cmd(p, f'ls {TEST_DIR_BASIC}', expected, msg, is_regex=True)


def assert_ls_1(p, files):
    expected = create_pattern(sorted(files), separate_lines=True)
    msg = 'Wrong content of listed directory, when calling `ls -1`'
    psh.assert_cmd(p, f'ls -1 {TEST_DIR_BASIC}', expected, msg, is_regex=True)


def assert_ls_a(p, files):
    expected = create_pattern(sorted(files))
    msg = 'Wrong content of listed directory, when calling `ls -a`'
    psh.assert_cmd(p, f'ls -a {TEST_DIR_BASIC}', expected, msg, is_regex=True)


def assert_ls_d(p):
    expected = create_pattern([TEST_DIR_BASIC, ])
    msg = 'Wrong output, when calling `ls -d tested_directory`'
    psh.assert_cmd(p, f'ls -d {TEST_DIR_BASIC}', expected, msg, is_regex=True)

    expected = create_pattern(['.', ])
    msg = 'Wrong output, when calling `ls -d` without specified directory'
    psh.assert_cmd(p, 'ls -d', expected, msg, is_regex=True)


def assert_ls_f(p, files):
    expected = create_pattern(files)
    msg = 'Wrong output, when calling `ls -f`'
    psh.assert_cmd(p, f'ls -f {TEST_DIR_BASIC}', expected, msg, is_regex=True)


def assert_ls_h(p):
    arg_help_pattern = r'(?P<help_line>\s*-[\w\s]:[ \S]+(\r+\n))+'
    expected = r'usage:(\s)ls \[options\] \[files\](\r+\n)' + arg_help_pattern
    msg = 'Wrong ls help message format'
    psh.assert_cmd(p, 'ls -h', expected, msg, is_regex=True)


def assert_ls_l(p, files):
    cmd = f'ls -l {TEST_DIR_BASIC}'
    p.sendline(cmd)
    p.expect(cmd)
    for file in sorted(files):
        if file[0] == '.':
            continue
        p.expect(r'(?P<ftype>-|b|c|d|l|n|p|s|D|E|O|S)(?P<permissions>([-r][-w][-x]){3}) +'
                 + r'(?P<hlink_cnt>\d) +'
                 + r'(?P<owners>(root |--- ){2}) *'
                 + r'(?P<size>\d+) '
                 + r'(?P<month>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) *'
                 + r'(?P<mday>( [1-9])|([1-2]\d)|(3[01])) +'
                 + r'(?P<hour>(\d){2})(:)(?P<min>(\d){2}) +'
                 + OPTIONAL_CONTROL_CODE
                 + f'{file}')

    psh.assert_prompt(p, "Prompt hasn't been seen after calling `ls -l`")


def assert_ls_r(p, files):
    expected = create_pattern(sorted(files, reverse=True))
    msg = 'Wrong output, when calling `ls -r`'
    psh.assert_cmd(p, f'ls -r {TEST_DIR_BASIC}', expected, msg, is_regex=True)


def assert_ls_S(p, files):
    # it's common that empty directories has larger size than empty files
    size_sorted_files = list(sorted(files, key=lambda x: (0, x) if x == 'dir' else (1, x)))
    expected = create_pattern(size_sorted_files)
    msg = 'Wrong output, when calling `ls -S`'
    psh.assert_cmd(p, f'ls -S {TEST_DIR_BASIC}', expected, msg, is_regex=True)


def harness(p):
    listed_files = ('test1', 'test0', 'dir')
    to_create_files = ('.hidden',) + listed_files
    all_files = ('.', '..') + to_create_files

    psh.init(p)

    create_testdir(p, ROOT_TEST_DIR)
    prepare_testdir(p, to_create_files)
    assert_ls_err(p)
    assert_ls_noarg(p, listed_files)
    assert_ls_1(p, listed_files)
    assert_ls_a(p, all_files)
    assert_ls_d(p)
    assert_ls_f(p, listed_files)
    assert_ls_h(p)
    assert_ls_l(p, listed_files)
    assert_ls_r(p, listed_files)
    assert_ls_S(p, listed_files)
