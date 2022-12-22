# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# process related tools for psh tests
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh


def get_process_list(p):
    ps_list = []
    oneline_pattern = r"(.*?)(\r+)\n"
    cmd = "ps"
    p.sendline(cmd)
    p.expect(cmd + psh.EOL)
    while True:
        idx = p.expect([psh.PROMPT, oneline_pattern])
        if idx == 0:
            break
        line = p.match.group(1)
        try:
            pid, ppid, pr, state, cpu, wait, time, vmem, thr, task = line.split(
                maxsplit=9
            )
        except ValueError:
            assert False, f"wrong ps output: {line}"

        if pid.isdigit():
            ps_list.append({"pid": pid, "task": task, "state": state})

    return ps_list


def create_psh_processes(p, count):
    def get_psh_pids(p):
        return [
            int(proc["pid"])
            for proc in get_process_list(p)
            if proc["task"] in ("psh", "/bin/psh")
        ]

    # Save already spawned psh processes - we don't want to kill them
    old_pids = get_psh_pids(p)

    for _ in range(count):
        psh.assert_exec(p, "psh", msg="Failed to create new psh process")

    new_pids = get_psh_pids(p)
    new_pids = [pid for pid in new_pids if pid not in old_pids]
    new_pids.sort(reverse=True)

    assert (
        len(new_pids) == count
    ), f"Created {len(new_pids)} psh processes, instead of {count}"
    return new_pids
