# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh exit command test
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh
from psh.tools.proc import get_process_list, create_psh_processes


def _count_sleep_pshs(p):
    return sum(
        (proc["state"] == "sleep" and proc["task"] in ("psh", "/bin/psh"))
        for proc in get_process_list(p)
    )


def assert_one_proc(p, already_spawned, args=False):
    psh.assert_exec(
        p,
        "psh",
        msg="Prompt hasn't been seen when creating new psh process using runfile",
    )
    assert (
        _count_sleep_pshs(p) == already_spawned + 1
    ), "New psh process not seen after launching psh by runfile!"
    cmd = "exit" if not args else "exit arg1 arg2 arg3 !@$#@$%^"
    psh.assert_cmd(
        p,
        cmd,
        result="success",
        msg="Prompt hasn't been seen after first exit (two psh processes should be run)",
    )

    assert (
        _count_sleep_pshs(p) == already_spawned
    ), "The count of sleepy psh processes haven't been decreased after calling exit!"


def assert_multi_procs(p, count, already_spawned):

    create_psh_processes(p, count)
    assert (
        _count_sleep_pshs(p) == already_spawned + count
    ), f"{count} new psh processes not seen after launching pshs by runfile!"

    for _ in range(count):
        psh.assert_cmd(
            p,
            cmd="exit",
            result="success",
            msg="Prompt hasn't been seen after first exit (two psh processes should be run)",
        )

    assert (
        _count_sleep_pshs(p) == already_spawned
    ), f"The count of sleepy psh processes haven't been decreased {count} times after calling exits!"


@psh.run
def harness(p):
    sleep_pshs = _count_sleep_pshs(p)

    assert_one_proc(p, already_spawned=sleep_pshs)
    assert_one_proc(p, already_spawned=sleep_pshs, args=True)
    assert_multi_procs(p, 3, already_spawned=sleep_pshs)
