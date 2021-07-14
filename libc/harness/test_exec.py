# Phoenix-RTOS
#
# libc-tests
#
# various exec functions tests
#
# Copyright 2021 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

from psh.tools.basic import run_psh, assert_only_prompt, assert_expected


def assert_execve_env_changed(p):
    cmd = '/bin/test_exec 1'
    expected = (r'(argc = 1(\r+)\n)'
                r'(argv\[0\] = /bin/to_exec(\r+)\n)'
                r'(environ\[0\] = TEST1=exec_value(\r+)\n)'
                r'(\r\x1b\[0J\(psh\)%)')

    assert_expected(p, cmd, expected, "Wrong output of execve function with changed environment")


def assert_execve_env_unchanged(p):
    cmd = '/bin/test_exec 2'
    expected = (r'(argc = 1(\r+)\n)'
                r'(argv\[0\] = /bin/to_exec(\r+)\n)'
                r'(environ\[0\] = TEST1=unchanged_value(\r+)\n)'
                r'(\r\x1b\[0J\(psh\)%)')

    assert_expected(p, cmd, expected, "Wrong output of execve function with unchanged environment")


def assert_execve_path_searched(p):
    cmd = '/bin/test_exec 3'
    expected = (r'(argc = 1(\r+)\n)'
                r'(argv\[0\] = to_exec(\r+)\n)'
                r'(environ\[0\] = PATH=/bin:/sbin:/usr/bin:/usr/sbin(\r+)\n)'
                r'(\r\x1b\[0J\(psh\)%)')

    assert_expected(p, cmd, expected, "Wrong output of execve function with searching in PATH environment variable")


def assert_execvpe_env_changed(p):
    cmd = '/bin/test_exec 4'
    expected = (r'(argc = 1(\r+)\n)'
                r'(argv\[0\] = /bin/to_exec(\r+)\n)'
                r'(environ\[0\] = TEST1=exec_value(\r+)\n)'
                r'(\r\x1b\[0J\(psh\)%)')

    assert_expected(p, cmd, expected, "Wrong output of execvpe function with changed environment")


def assert_execvpe_env_unchanged(p):
    cmd = '/bin/test_exec 5'
    expected = (r'(argc = 1(\r+)\n)'
                r'(argv\[0\] = /bin/to_exec(\r+)\n)'
                r'(environ\[0\] = TEST1=unchanged_value(\r+)\n)'
                r'(\r\x1b\[0J\(psh\)%)')

    assert_expected(p, cmd, expected, "Wrong output of execvpe function with unchanged environment")


def assert_execvpe_path_searched(p):
    cmd = '/bin/test_exec 6'
    expected = (r'(argc = 1(\r+)\n)'
                r'(argv\[0\] = to_exec(\r+)\n)'
                r'(environ\[0\] = PATH=/bin:/sbin:/usr/bin:/usr/sbin(\r+)\n)'
                r'(\r\x1b\[0J\(psh\)%)')

    assert_expected(p, cmd, expected, "Wrong output of execvpe function with searching in PATH environment variable")


def assert_execvp_env_unchanged(p):
    cmd = '/bin/test_exec 7'
    expected = (r'(argc = 1(\r+)\n)'
                r'(argv\[0\] = /bin/to_exec(\r+)\n)'
                r'(environ\[0\] = TEST1=unchanged_value(\r+)\n)'
                r'(\r\x1b\[0J\(psh\)%)')

    assert_expected(p, cmd, expected, "Wrong output of execvp function with unchanged environment")


def assert_execvp_path_searched(p):
    cmd = '/bin/test_exec 8'
    expected = (r'(argc = 1(\r+)\n)'
                r'(argv\[0\] = to_exec(\r+)\n)'
                r'(environ\[0\] = PATH=/bin:/sbin:/usr/bin:/usr/sbin(\r+)\n)'
                r'(\r\x1b\[0J\(psh\)%)')

    assert_expected(p, cmd, expected, "Wrong output of execvp function with searching in PATH environment variable")


def harness(p):
    run_psh(p)
    assert_only_prompt(p)

    assert_execve_env_changed(p)
    assert_execve_env_unchanged(p)
    assert_execve_path_searched(p)
    assert_execvpe_env_changed(p)
    assert_execvpe_env_unchanged(p)
    assert_execvpe_path_searched(p)
    assert_execvp_env_unchanged(p)
    assert_execvp_path_searched(p)
