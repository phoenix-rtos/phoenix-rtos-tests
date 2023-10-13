
# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# pshlogin test
#
# Copyright 2021 Phoenix Systems
# Author: Mateusz Niewiadomski
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#
import string

import pexpect

import psh.tools.psh as psh

import psh.tools.login as logintools


EOT = '\x04'
BACKSPACE = '\x08'
cred_ok = logintools.Credentials('defuser', '1234')
cred_ok_etc = logintools.Credentials('root', '1234')
cred_bad = logintools.Credentials('lorem', 'ipsum')
cred_empty = logintools.Credentials('', '')


def assert_pshlogin(p):
    p.send(logintools.pshlogin_cmd + '\n')
    p.expect_exact(logintools.pshlogin_cmd)
    idx = p.expect_exact(['Login:', '(psh)%', pexpect.TIMEOUT], timeout=1)
    if idx == 1:
        raise AssertionError('pshlogin is not available!')
    if idx == 2:
        raise AssertionError('pshlogin does not launch')


@psh.run
def harness(p):
    # Run pshlogin
    assert_pshlogin(p)

    # Wrong credentials
    logintools.assert_pshlogin_fail(p, cred_ok_etc.user, cred_bad.passwd)  # good login, wrong password
    logintools.assert_pshlogin_fail(p, cred_bad.user, cred_ok_etc.passwd)  # bad login, good password
    logintools.assert_pshlogin_fail(p, cred_bad.user, cred_bad.passwd)  # bad login, bad password

    # Empty Credentials
    logintools.assert_pshlogin_fail(p, cred_ok_etc.user, cred_empty.passwd)  # good login, empty password
    logintools.assert_pshlogin_fail(p, cred_bad.user, cred_empty.passwd)  # bad login, empty password
    logintools.assert_login_empty(p, cred_empty.user)  # empty login, good password

    # Good login, new psh is created
    logintools.assert_login(p, cred_ok_etc.user, cred_ok_etc.passwd)

    # Exit and try to login again but with defuser:
    p.send(EOT)
    psh.assert_prompt(p)

    assert_pshlogin(p)
    logintools.assert_pshlogin_fail(p, cred_ok.user, cred_bad.passwd)
    logintools.assert_login(p, cred_ok.user, cred_ok.passwd)

    p.send(EOT)
    psh.assert_prompt(p)

    # Too long credentials
    assert_pshlogin(p)
    p.send(string.printable + '\n')
    assert p.expect_exact(['Login:', pexpect.TIMEOUT], timeout=2) == 0, 'Long login does not repeat login procedure'
    logintools.assert_pshlogin_fail(p, cred_ok.user, string.printable)  # too long password

    # Proper login to go back to psh
    logintools.assert_login(p, cred_ok_etc.user, cred_ok_etc.passwd)
    p.send(EOT)
