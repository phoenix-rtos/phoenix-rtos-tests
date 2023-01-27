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
from time import sleep


def get_process_list(p):
    ps_list = []
    oneline_pattern = r'(.*?)(\r+)\n'
    cmd = 'ps'
    p.sendline(cmd)
    p.expect(cmd + psh.EOL)
    while True:
        idx = p.expect([psh.PROMPT, oneline_pattern])
        if idx == 0:
            break
        line = p.match.group(1)
        try:
            pid, ppid, pr, state, cpu, wait, time, vmem, thr, task = line.split(maxsplit=9)
        except ValueError:
            assert False, f'wrong ps output: {line}'

        if pid.isdigit():
            ps_list.append({'pid': pid, 'task': task, 'state': state})

    return ps_list


def create_psh_processes(p, count):
    def get_psh_pids(p):
        return [int(proc["pid"]) for proc in get_process_list(p) if proc["task"] in ("psh", "/bin/psh")]

    # Save spawned already spawned psh processes - we don't want to kill them
    old_pids = get_psh_pids(p)

    for _ in range(count):
        psh.assert_exec(p, 'psh', msg='Failed to create new psh process')

    new_pids = get_psh_pids(p)

    new_pids = [pid for pid in new_pids if pid not in old_pids]
    new_pids.reverse()

    assert len(new_pids) == count, f'Created {len(new_pids)} psh processes, instead of {count}'
    return new_pids


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
    psh.assert_cmd(p, f'kill {unused_pid}', result="fail", msg=msg)

    msg = 'Wrong kill help message'
    psh.assert_cmd(p, 'kill', expected='usage: kill <pid>', result='fail', msg=msg)
    msg = 'Wrong kill error message when passing string in argument'
    psh.assert_cmd(p, 'kill not_number',
                   expected='kill: could not parse process id: not_number',
                   result='fail',
                   msg=msg)


@psh.run
def harness(p):
    assert_errors(p)

    pid_list = create_psh_processes(p, 3)
    assert_kill_procs(p, pid_list)
