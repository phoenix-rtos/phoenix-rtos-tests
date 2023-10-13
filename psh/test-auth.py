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

import string
import pexpect

import psh.tools.psh as psh

import psh.tools.login as logintools

from psh.tools.psh import EOT


BACKSPACE = '\x08'
cred_ok = logintools.Credentials('defuser', '1234')
cred_bad = logintools.Credentials('lorem', 'ipsum')
cred_empty = logintools.Credentials('', '')


def assert_auth(p):
    p.send('auth\n')
    assert p.expect_exact(['auth', pexpect.TIMEOUT], timeout=1) == 0, 'Error on entering command auth'
    assert p.expect_exact(['Login:', pexpect.TIMEOUT], timeout=2) == 0, '"auth" applet does not launch'


@psh.run
def harness(p):
    # Check if auth app is available and login exit
    assert_auth(p)
    p.send(EOT)
    psh.assert_prompt(p, msg='Cannot exit "auth" during login passing', timeout=1)

    # Exiting auth during password
    assert_auth(p)
    p.send(cred_bad.user + '\n')
    p.send(EOT)
    psh.assert_prompt(p, msg='Cannot exit "auth": password passing', timeout=1)

    # Good login
    assert_auth(p)
    logintools.assert_login(p, cred_ok.user, cred_ok.passwd)

    # Wrong credentials
    assert_auth(p)
    logintools.assert_auth_fail(p, cred_ok.user, cred_bad.passwd)

    assert_auth(p)
    logintools.assert_auth_fail(p, cred_bad.user, cred_ok.passwd)
    assert_auth(p)
    logintools.assert_auth_fail(p, cred_bad.user, cred_bad.passwd)

    # Empty Credentials
    assert_auth(p)
    logintools.assert_auth_fail(p, cred_ok.user, cred_empty.passwd)
    assert_auth(p)
    logintools.assert_auth_fail(p, cred_bad.user, cred_empty.passwd)
    assert_auth(p)
    logintools.assert_login_empty(p, cred_empty.user)
    p.send(EOT)  # assert_login_empty() with empty login does not go back to psh, exit needed
    psh.assert_prompt(p, msg='Cannot exit "auth" during empty login passing', timeout=1)

    # Too long credentials
    printables = "".join(string.printable.split())  # remove whitespaces
    assert_auth(p)
    p.send(printables + '\n')
    assert p.expect_exact(['Login:', pexpect.TIMEOUT]) == 0, 'Long login does not repeat login procedure'
    logintools.assert_auth_fail(p, cred_ok.user, printables)  # too long password

    # Test backspace
    assert_auth(p)
    p.send(cred_ok.user + cred_ok.user)
    for i in range(len(cred_ok.user) + 1):  # Test just enough backspaces
        p.send(BACKSPACE)
    p.send(cred_ok.user[-1] + '\n')
    assert p.expect_exact(["Password:", pexpect.TIMEOUT]) == 0
    p.send(cred_ok.passwd)
    for i in range(len(cred_ok.passwd) + 9):  # test too many backspaces
        p.send(BACKSPACE)
    p.send(cred_ok.passwd + '\n')
    psh.assert_prompt(p, msg='Login should pass but failed', timeout=1)
    psh.assert_cmd_successed(p)
