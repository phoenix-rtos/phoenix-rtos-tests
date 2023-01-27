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

import psh.tools.psh as psh


def assert_execve_env_changed(p):
    cmd = '/bin/test-exec 1'
    expected = (('argc = 1',
                 'argv[0] = /bin/to_exec',
                 'environ[0] = TEST1=exec_value'))
    msg = "Wrong output of execve function with changed environment"

    psh.assert_cmd(p, cmd, expected=expected, result='success', msg=msg)


def assert_execve_env_unchanged(p):
    cmd = '/bin/test-exec 2'
    expected = (('argc = 1',
                 'argv[0] = /bin/to_exec',
                 'environ[0] = TEST1=unchanged_value'))
    msg = "Wrong output of execve function with unchanged environment"

    psh.assert_cmd(p, cmd, expected=expected, result='success', msg=msg)


def assert_execve_path_searched(p):
    cmd = '/bin/test-exec 3'
    expected = (('argc = 1',
                 'argv[0] = to_exec',
                 'environ[0] = PATH=/bin:/sbin:/usr/bin:/usr/sbin'))
    msg = "Wrong output of execve function with searching in PATH environment variable"

    psh.assert_cmd(p, cmd, expected=expected, result='success', msg=msg)


def assert_execvpe_env_changed(p):
    cmd = '/bin/test-exec 4'
    expected = (('argc = 1',
                 'argv[0] = /bin/to_exec',
                 'environ[0] = TEST1=exec_value'))
    msg = "Wrong output of execvpe function with changed environment"

    psh.assert_cmd(p, cmd, expected=expected, result='success', msg=msg)


def assert_execvpe_env_unchanged(p):
    cmd = '/bin/test-exec 5'
    expected = (('argc = 1',
                 'argv[0] = /bin/to_exec',
                 'environ[0] = TEST1=unchanged_value'))
    msg = "Wrong output of execvpe function with unchanged environment"

    psh.assert_cmd(p, cmd, expected=expected, result='success', msg=msg)


def assert_execvpe_path_searched(p):
    cmd = '/bin/test-exec 6'
    expected = (('argc = 1',
                 'argv[0] = to_exec',
                 'environ[0] = PATH=/bin:/sbin:/usr/bin:/usr/sbin'))
    msg = "Wrong output of execvpe function with searching in PATH environment variable"

    psh.assert_cmd(p, cmd, expected=expected, result='success', msg=msg)


def assert_execvp_env_unchanged(p):
    cmd = '/bin/test-exec 7'
    expected = (('argc = 1',
                 'argv[0] = /bin/to_exec',
                 'environ[0] = TEST1=unchanged_value'))
    msg = "Wrong output of execvp function with unchanged environment"

    psh.assert_cmd(p, cmd, expected=expected, result='success', msg=msg)


def assert_execvp_path_searched(p):
    cmd = '/bin/test-exec 8'
    expected = (('argc = 1',
                 'argv[0] = to_exec',
                 'environ[0] = PATH=/bin:/sbin:/usr/bin:/usr/sbin'))
    msg = "Wrong output of execvp function with searching in PATH environment variable"

    psh.assert_cmd(p, cmd, expected=expected, result='success', msg=msg)


def harness(p):
    psh.init(p)

    assert_execve_env_changed(p)
    assert_execve_env_unchanged(p)
    assert_execve_path_searched(p)
    assert_execvpe_env_changed(p)
    assert_execvpe_env_unchanged(p)
    assert_execvpe_path_searched(p)
    assert_execvp_env_unchanged(p)
    assert_execvp_path_searched(p)

    p.sendline("exit")
