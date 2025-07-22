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


ROOT_TEST_DIR = "/tmp/test_perf_trace"


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


def assert_perf_basic(p):
    traceDir = ROOT_TEST_DIR
    psh.assert_prompt_after_cmd(
        p, f"perf -m trace -o {traceDir} -t 500", result='success')
    files = psh.ls(p, traceDir)
    assert_present("channel_event0", files, dir=False)
    assert_present("channel_meta0", files, dir=False)


@psh.run
def harness(p):
    assert_dir_created(p, ROOT_TEST_DIR)

    assert_perf_h(p)
    assert_perf_err(p)
    assert_perf_basic(p)

    assert_deleted_rec(p, ROOT_TEST_DIR)
