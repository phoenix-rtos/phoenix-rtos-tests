# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# simple psh uptime command test
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

from time import sleep
import psh.tools.psh as psh


def assert_format(p):
    msg = 'Wrong output format when calling `uptime` without arguments'
    psh.assert_cmd(p, 'uptime', r'up (\d day(s)? and )?\d{2}:\d{2}:\d{2}(\r+)\n', msg, is_regex=True)

    msg = 'Wrong output format when calling `uptime` with an argument'
    psh.assert_cmd(p, 'uptime -s', r'\d+(\r+)\n', msg, is_regex=True)


def assert_greater(p):
    # assumes that the time since the test start is lower than one minute
    time1 = psh.uptime(p)
    sleep(2)
    time2 = psh.uptime(p)

    assert time2 > time1, "The uptime value isn't greater than 2 seconds before"


def harness(p):
    psh.init(p)

    assert_format(p)
    assert_greater(p)
