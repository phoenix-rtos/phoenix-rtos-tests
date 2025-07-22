# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh perf command test
#
# Copyright 2025 Phoenix Systems
# Author: Adam Greloch
#
# %LICENSE%
#

import psh.tools.psh as psh
from psh.tools.common import assert_file_created, assert_file_deleted, \
    assert_present, assert_dir_created, assert_deleted_rec
from time import sleep


ROOT_TEST_DIR = "/tmp/test_perf_trace"
START_STOP_REPEATS = 3


def assert_perf_h(p):
    psh.assert_prompt_after_cmd(p, "perf", result='fail')
    psh.assert_prompt_after_cmd(p, "perf -h", result='success')


def assert_perf_err(p):
    # should also pass dest directory with `-o` when not compiled
    # with RTT support
    psh.assert_prompt_after_cmd(p, "perf -m trace", result='fail')

    fname = ROOT_TEST_DIR + "/notadir"
    assert_file_created(p, fname)
    psh.assert_prompt_after_cmd(p, f"perf -m trace -o {fname}", result='fail')
    assert_file_deleted(p, fname)


def assert_trace_output_present(p, path):
    files = psh.ls(p, path)
    assert_present("channel_event0", files, dir=False)
    assert_present("channel_meta0", files, dir=False)


def assert_perf_basic(p):
    traceDir1 = ROOT_TEST_DIR + "/trace_basic"
    psh.assert_prompt_after_cmd(
        p, f"perf -m trace -o {traceDir1} -t 500", result='success')
    assert_trace_output_present(p, traceDir1)


def assert_perf_start_stop(p):
    for _ in range(START_STOP_REPEATS):
        traceDir = ROOT_TEST_DIR + "/trace_start_stop"
        psh.assert_prompt_after_cmd(p, f"perf -m trace -o {traceDir} -j stop", result='fail')

        psh.assert_prompt_after_cmd(p, f"perf -m trace -o {traceDir} -j start", result='success')
        psh.assert_prompt_after_cmd(p, f"perf -m trace -o {traceDir} -j start", result='fail')

        sleep(0.5)

        psh.assert_prompt_after_cmd(p, f"perf -m trace -o {traceDir} -j stop", result='success')
        psh.assert_prompt_after_cmd(p, f"perf -m trace -o {traceDir} -j stop", result='fail')

        assert_trace_output_present(p, traceDir)


@psh.run
def harness(p):
    assert_dir_created(p, ROOT_TEST_DIR)

    assert_perf_h(p)
    assert_perf_err(p)
    assert_perf_basic(p)
    assert_perf_start_stop(p)

    assert_deleted_rec(p, ROOT_TEST_DIR)
