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

esc_codes = {
    'set_blue': r'\x1b\[34m',
    'set_green': r'\033\[32m',
    'set_yellowonblack': r'\033\[33;40m',
    'reset_attributes': r'\x1b\[0m'
}


def create_testdir(p, files):
    # no assertion for creating directory properly, because testdir may already exists (no rm command)
    msg = 'Prompt not seen after calling `mkdir test_ls_dir`'
    psh.assert_cmd(p, 'mkdir test_ls_dir', r'.*?(\r+\n)?', msg, is_regex=True)

    cmd = 'touch'
    for file in files:
        if file == 'dir':
            continue
        cmd += f' test_ls_dir/{file}'

    msg = f'Prompt not seen after calling `{cmd}`'
    psh.assert_cmd(p, cmd, '', msg)

    msg = 'Prompt not seen after calling `mkdir test_ls_dir/dir`'
    psh.assert_cmd(p, 'mkdir test_ls_dir/dir', r'.*?(\r+\n)?', msg, is_regex=True)


def assert_ls_err(p):
    msg = 'Wrong error message, when trying to print nonexistent directory content'
    psh.assert_cmd(p, 'ls nonexistentDir', "ls: can't access nonexistentDir: no such file or directory", msg)

    return True


def assert_ls_noarg(p, files):
    expected = r''
    for file in sorted(files):
        if file == 'dir':
            expected += esc_codes['set_blue']
        if file[0] == '.':
            continue
        # last position
        if file == sorted(files)[len(files)-1]:
            expected += f'{file}' + esc_codes['reset_attributes']
        else:
            expected += f'{file}  ' + esc_codes['reset_attributes']
    # `expected` has to be treated as regex in order not to omit esc sequences, so last EOL has to be matched too
    expected = expected + psh.EOL

    msg = 'Wrong content of listed directory, when calling ls without arguments'
    psh.assert_cmd(p, 'ls test_ls_dir', expected, msg, is_regex=True)


def assert_ls_1(p, files):
    expected = r''
    for file in sorted(files):
        if file == 'dir':
            expected += esc_codes['set_blue']
        if file[0] == '.':
            continue
        # last position
        if file == sorted(files)[len(files)-1]:
            expected += f'{file}' + esc_codes['reset_attributes']
        else:
            expected += f'{file}' + esc_codes['reset_attributes'] + psh.EOL
    expected = expected + psh.EOL

    msg = 'Wrong content of listed directory, when calling `ls -1`'
    psh.assert_cmd(p, 'ls -1 test_ls_dir', expected, msg, is_regex=True)


def assert_ls_a(p, files):
    expected = esc_codes['set_blue'] \
        + '.  ' \
        + esc_codes['reset_attributes'] \
        + esc_codes['set_blue'] \
        + '..  ' \
        + esc_codes['reset_attributes']

    for file in sorted(files):
        if file == 'dir':
            expected += esc_codes['set_blue']
        # last position
        if file == sorted(files)[len(files)-1]:
            expected += f'{file}' + esc_codes['reset_attributes']
        else:
            expected += f'{file}  ' + esc_codes['reset_attributes']
    expected = expected + psh.EOL

    msg = 'Wrong content of listed directory, when calling `ls -a`'
    psh.assert_cmd(p, 'ls -a test_ls_dir', expected, msg, is_regex=True)


def assert_ls_d(p):
    expected = esc_codes['set_blue'] + 'test_ls_dir' + esc_codes['reset_attributes']
    expected = expected + psh.EOL

    msg = 'Wrong output, when calling `ls -d tested_directory`'
    psh.assert_cmd(p, 'ls -d test_ls_dir', expected, msg, is_regex=True)


def assert_ls_d_noarg(p):
    expected = esc_codes['set_blue'] \
        + '.' \
        + esc_codes['reset_attributes']
    expected = expected + psh.EOL

    msg = 'Wrong output, when calling `ls -d` without specified directory'
    psh.assert_cmd(p, 'ls -d', expected, msg, is_regex=True)


def assert_ls_f(p, files):
    expected = ''
    for file in files:
        if file == 'dir':
            expected += esc_codes['set_blue']
        if file[0] == '.':
            continue
        # last position
        if file == files[len(files)-1]:
            expected += f'{file}' + esc_codes['reset_attributes']
        else:
            expected += f'{file}  ' + esc_codes['reset_attributes']
    expected = expected + psh.EOL

    msg = 'Wrong output, when calling `ls -f test_ls_dir`'
    psh.assert_cmd(p, 'ls -f test_ls_dir', expected, msg, is_regex=True)


def assert_ls_h(p):
    expected = r'usage:(\s)ls \[options\] \[files\](\r+\n)' + \
                r'(\s+-[\w\s]:[ \S]+(\r+\n))+\s+-[\w\s]:[ \S]+'
    expected = expected + psh.EOL

    msg = 'Wrong ls help message format'
    psh.assert_cmd(p, 'ls -h', expected, msg, is_regex=True)


def assert_ls_l(p, files):
    cmd = 'ls -l test_ls_dir'
    p.sendline(cmd)
    p.expect(cmd)
    for file in sorted(files):
        if file[0] == '.':
            continue
        p.expect(r'(-|b|c|d|l|n|p|s|D|E|O|S)([-r][-w][-x]){3}( )'
                 + r'(\d)( )'
                 + r'(root |--- ){2}( *)'
                 + r'(\d+)( )'
                 + r'(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)( )'
                 + r'[1-31]( *)'
                 + r'(\d){2}(:)(\d){2}( )'
                 + r'(\x1b\[34m)?'
                 + f'{file}')

    psh.assert_prompt(p, "Prompt hasn't been seen after calling `ls -l test_ls_dir`")


def assert_ls_r(p, files):
    expected = ''
    hidd_files_nr = 1
    for file in sorted(files, reverse=True):
        if file == 'dir':
            expected += esc_codes['set_blue']
        if file[0] == '.':
            continue
        # last position
        if file == sorted(files, reverse=True)[len(files)-1-hidd_files_nr]:
            expected += f'{file}' + esc_codes['reset_attributes']
        else:
            expected += f'{file}  ' + esc_codes['reset_attributes']
    expected = expected + psh.EOL

    msg = 'Wrong output, when calling `ls -r test_ls_dir`'
    psh.assert_cmd(p, 'ls -r test_ls_dir', expected, msg, is_regex=True)


def assert_ls_S(p, files):
    # dir size > empty files size
    expected = esc_codes['set_blue'] + 'dir  ' + esc_codes['reset_attributes']

    for file in sorted(files):
        if file == 'dir':
            continue
        if file[0] == '.':
            continue
        if file == sorted(files)[len(files)-1]:
            expected += f'{file}' + esc_codes['reset_attributes']
        else:
            expected += f'{file}  ' + esc_codes['reset_attributes']
    expected = expected + psh.EOL

    msg = 'Wrong output, when calling `ls -S test_ls_dir`'
    psh.assert_cmd(p, 'ls -S test_ls_dir', expected, msg, is_regex=True)


def assert_ls_devcolor(p):
    # the following regex will match list of various devices
    expected = f'(({esc_codes["set_yellowonblack"]}' \
               + r'(\S+)[ \t]*' \
               + f'{esc_codes["reset_attributes"]}' \
               + r')+' \
               + r'(\r+\n)' \
               + r')+'

    msg = 'Devices colors are not printed correctly when calling `ls dev`'
    psh.assert_cmd(p, 'ls dev', expected, msg, is_regex=True)


def harness(p):
    # this tuple shouldn't be changed
    files = ('.hidden', 'test1', 'test0', 'dir')

    psh.init(p)

    create_testdir(p, files)

    assert_ls_err(p)
    assert_ls_noarg(p, files)
    assert_ls_1(p, files)
    assert_ls_a(p, files)
    assert_ls_d(p)
    assert_ls_d_noarg(p)
    assert_ls_f(p, files)
    assert_ls_h(p)
    assert_ls_l(p, files)
    assert_ls_r(p, files)
    assert_ls_S(p, files)
    assert_ls_devcolor(p)
