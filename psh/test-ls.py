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
from psh.tools.psh import OPTIONAL_CONTROL_CODE
from psh.tools.common import SEPARATOR_PATTERN, create_testdir, assert_deleted_rec

FNAME_PATTERN = r'(?P<fname>\S+)'

ROOT_TEST_DIR = 'test_ls_dir'
TEST_DIR_BASIC = f'{ROOT_TEST_DIR}/basic'


def prepare_testdir(p, files):
    msg = 'Wrong output when creating basic directory!'
    psh.assert_cmd(p, f'mkdir {TEST_DIR_BASIC}', result='success', msg=msg)
    cmd = 'touch'
    for file in files:
        if file == 'dir':
            continue
        cmd += f' {TEST_DIR_BASIC}/{file}'

    msg = 'Wrong output when creating basic test files!'
    psh.assert_cmd(p, cmd, result='success', msg=msg)

    msg = 'Wrong output when creating internal test directory!'
    psh.assert_cmd(p, f'mkdir {TEST_DIR_BASIC}/dir', result='success', msg=msg)


def create_unordered_pattern(files):
    """ Create regex pattern for ls output
    containing all files from the `files` list in any order """

    filesCnt = len(files)
    files = '|'.join(files)
    pattern = f'({OPTIONAL_CONTROL_CODE}({files}){SEPARATOR_PATTERN})'
    pattern += rf'{{{filesCnt}}}'

    # ( optional CC + (file1|file2|filen) + ((wss) + CC)|(CC + wss)|(wss)) ){n}
    # wss - white spaces, CC - control code
    return pattern


def create_ordered_pattern(files, separate_lines=False):
    """ Create regex pattern for ls output
    containing all files from the `files` list in the same order """

    newline_separator = OPTIONAL_CONTROL_CODE + r'(\r+\n)'
    sep = newline_separator if separate_lines else SEPARATOR_PATTERN
    files = [OPTIONAL_CONTROL_CODE + file for file in files]
    pattern = sep.join(files) + sep

    # optional CC + file1 + ((wss) + CC)|(CC + wss)|(wss)) and same for next files
    return pattern


def assert_ls_err(p):
    msg = 'Wrong error message, when trying to print nonexistent directory content'
    expected = "ls: can't access nonexistentDir: no such file or directory"
    psh.assert_cmd(p, 'ls nonexistentDir', expected=expected, result='fail', msg=msg)


def assert_ls_noarg(p, files):
    expected = create_ordered_pattern(sorted(files))
    msg = 'Wrong content of listed directory, when calling ls without arguments'
    psh.assert_cmd(p, f'ls {TEST_DIR_BASIC}', expected=expected, result='success', msg=msg, is_regex=True)


def assert_ls_1(p, files):
    expected = create_ordered_pattern(sorted(files), separate_lines=True)
    msg = 'Wrong content of listed directory, when calling `ls -1`'
    psh.assert_cmd(p, f'ls -1 {TEST_DIR_BASIC}', expected=expected, result='success', msg=msg, is_regex=True)


def assert_ls_a(p, files):
    expected = create_ordered_pattern(sorted(files))
    msg = 'Wrong content of listed directory, when calling `ls -a`'
    psh.assert_cmd(p, f'ls -a {TEST_DIR_BASIC}', expected=expected, result='success', msg=msg, is_regex=True)


def assert_ls_d(p):
    expected = create_ordered_pattern([TEST_DIR_BASIC, ])
    msg = 'Wrong output, when calling `ls -d tested_directory`'
    psh.assert_cmd(p, f'ls -d {TEST_DIR_BASIC}', expected=expected, result='success', msg=msg, is_regex=True)

    expected = create_ordered_pattern(['.', ])
    msg = 'Wrong output, when calling `ls -d` without specified directory'
    psh.assert_cmd(p, 'ls -d', expected=expected, result='success', msg=msg, is_regex=True)


def assert_ls_f(p, files):
    expected = create_unordered_pattern(files)
    msg = 'Wrong output, when calling `ls -f`'
    psh.assert_cmd(p, f'ls -f {TEST_DIR_BASIC}', expected=expected, result='success', msg=msg, is_regex=True)


def assert_ls_h(p):
    arg_help_pattern = r'(?P<help_line>\s*-[\w\s]:[ \S]+(\r+\n))+'
    expected = r'usage:(\s)ls \[options\] \[files\](\r+\n)' + arg_help_pattern
    msg = 'Wrong ls help message format'
    psh.assert_cmd(p, 'ls -h', expected=expected, result='success', msg=msg, is_regex=True)


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
    expected = create_ordered_pattern(sorted(files, reverse=True))
    msg = 'Wrong output, when calling `ls -r`'
    psh.assert_cmd(p, f'ls -r {TEST_DIR_BASIC}', expected=expected, result='success', msg=msg, is_regex=True)


@psh.run
def harness(p):
    listed_files = ('test1', 'test0', 'dir')
    to_create_files = ('.hidden',) + listed_files
    all_files = ('.', '..') + to_create_files

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

    assert_deleted_rec(p, ROOT_TEST_DIR)
