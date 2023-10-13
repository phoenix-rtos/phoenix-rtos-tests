#
# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# tools for login related testing
#
# Copyright 2021 Phoenix Systems
# Author: Mateusz Niewiadomski
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#
from collections import namedtuple

import pexpect

import psh.tools.psh as psh


Credentials = namedtuple('Credentials', 'user passwd')
pshlogin_cmd = '/bin/pshlogin'


def log_in(p, login, passwd):
    p.send(login + '\n')
    assert p.expect_exact([login, pexpect.TIMEOUT]) == 0, 'Cannot enter login to login prompt'
    assert p.expect_exact(["Password:", pexpect.TIMEOUT]) == 0
    p.send(passwd + '\n')


def assert_login(p, login, passwd):
    log_in(p, login, passwd)
    psh.assert_prompt(p, msg='Login should pass but failed (or hasn\'t been completed in 2s)', timeout=2)
    psh.assert_cmd_successed(p)


def assert_pshlogin_fail(p, login, passwd):
    log_in(p, login, passwd)
    # prompt should not been seen after passing wrong data when logging in using pshapp
    psh.assert_prompt_fail(p, msg='Login should fail but passed (prompt appeared in 2s)', timeout=2)


def assert_auth_fail(p, login, passwd):
    log_in(p, login, passwd)
    # prompt should been seen even after passing wrong data when logging in using auth applet
    psh.assert_prompt(p, 'Prompt not seen after an attempt to log in')
    psh.assert_cmd_failed(p)


def assert_login_empty(p, login):
    p.send(login + '\n')
    assert p.expect_exact(['Login:', pexpect.TIMEOUT]) == 0, 'Empty login doesn`t repeat logging'
