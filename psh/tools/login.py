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
import pexpect

from collections import namedtuple

Credentials = namedtuple('Credentials', 'user passwd')
pshlogin_cmd = '/bin/pshlogin'


def log_in(p, login, passwd):
    p.send(login + '\n')
    assert p.expect_exact([login, pexpect.TIMEOUT]) == 0, 'Cannot enter login to login prompt'
    assert p.expect_exact(["Password:", pexpect.TIMEOUT]) == 0
    p.send(passwd + '\n')


def assert_failed_login(p):
    assert p.expect_exact(['(psh)%', pexpect.TIMEOUT], timeout=1) > 0, 'Login should fail but passed'


def assert_good_login(p):
    assert p.expect_exact(['(psh)%', pexpect.TIMEOUT], timeout=1) == 0, 'Login should pass but failed'


def assert_login(p, login, passwd):
    log_in(p, login, passwd)
    assert_good_login(p)


def assert_login_fail(p, login, passwd):
    log_in(p, login, passwd)
    assert_failed_login(p)


def assert_login_empty(p, login):
    p.send(login + '\n')
    assert p.expect_exact(['Login:', pexpect.TIMEOUT]) == 0, 'Empty login doesn`t repeat logging'
