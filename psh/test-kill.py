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
    psh_pid_list = []
    msg = 'Failed to create new psh process'
    for _ in range(count):
        psh.assert_exec(p, 'psh', '', msg)

    # find pid list of sleep psh processes
    for proc in get_process_list(p):
        task = proc['task']
        if proc['state'] == 'sleep' and (task == 'psh' or task == '/bin/psh'):
            psh_pid_list.append(proc['pid'])

    psh_pid_count = len(psh_pid_list)
    assert psh_pid_count == count, f'Created {psh_pid_count} psh processes, instead of {count}'

    return psh_pid_list


def assert_kill_procs(p, pid_list):
    for pid in pid_list:
        msg = f'Wrong output when killing process with the following pid: {pid}'
        psh.assert_cmd(p, f'kill {pid}', '', msg)
    sleep(0.5)

    dead = set(pid_list) - set(proc['pid'] for proc in get_process_list(p))
    assert len(dead) == len(pid_list), f'failed to kill {len(pid_list) - len(dead)} processes out of {len(pid_list)}'


def assert_errors(p):
    pid = 90
    used_pid_list = [proc["pid"] for proc in get_process_list(p)]

    # if process id is used, find next unused id
    while str(pid) in used_pid_list:
        pid = pid + 1

    msg = 'Wrong kill help message'
    psh.assert_cmd(p, 'kill', 'usage: kill <pid>', msg)
    msg = 'Wrong kill error message when passing string in argument'
    psh.assert_cmd(p, 'kill not_number', 'kill: could not parse process id: not_number', msg)

    # nothing should be done
    msg = 'Wrong output when killing nonexistent process'
    psh.assert_cmd(p, f'kill {pid}', '', msg)


def harness(p):
    psh.init(p)

    assert_errors(p)

    pid_list = create_psh_processes(p, 3)
    assert_kill_procs(p, pid_list)
