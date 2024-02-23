# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh kill command test
#
# Copyright 2021, 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh
from psh.tools.proc import get_process_list, create_psh_processes
from time import sleep


def assert_kill_procs(p, pid_list):
    for pid in pid_list:
        msg = f'Wrong output when killing process with the following pid: {pid}'
        psh.assert_cmd(p, f'kill {pid}', result='dont-check', msg=msg)

    sleep(0.5)

    dead = set(pid_list) - set(proc['pid'] for proc in get_process_list(p))
    assert len(dead) == len(pid_list), f'failed to kill {len(pid_list) - len(dead)} processes out of {len(pid_list)}'


def assert_errors(p):
    unused_pid = max(int(proc["pid"]) for proc in get_process_list(p)) + 1

    # nothing should be done
    msg = 'Wrong output when killing nonexistent process'
    psh.assert_cmd(p,
                   f'kill {unused_pid}',
                   expected=rf'(kill: failed to send signal to process \d+?{psh.EOL})|{psh.EOL}',
                   result="fail",
                   msg=msg,
                   is_regex=True)

    msg = 'Wrong kill help message'
    psh.assert_cmd(p, 'kill',
                   expected=rf'(kill: |)(u|U)sage: kill[^\r]+?{psh.EOL}',
                   result='fail',
                   msg=msg,
                   is_regex=True)

    msg = 'Wrong kill error message when passing string in argument'
    psh.assert_cmd(p, 'kill not_number',
                   expected=rf'kill: [^\r]+?{psh.EOL}',
                   result='fail',
                   msg=msg,
                   is_regex=True)


@psh.run
def harness(p):
    assert_errors(p)

    pid_list = create_psh_processes(p, 3)
    assert_kill_procs(p, pid_list)
