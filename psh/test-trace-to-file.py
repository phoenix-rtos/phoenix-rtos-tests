# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh perf command test - save trace to fs
#
# Copyright 2025 Phoenix Systems
# Author: Adam Greloch
#
# %LICENSE%
#

import psh.tools.psh as psh
from psh.tools.common import assert_present, assert_dir_created


# NOTE: path expected in CI - the emitted trace is later captured by QEMU
# host for correctness validation
TRACE_DIR = "/test_perf_trace"


@psh.run
def harness(p):
    assert_dir_created(p, TRACE_DIR)

    traceDir = TRACE_DIR
    psh.assert_prompt_after_cmd(
        p, f"perf -m trace -o {traceDir} -t 1000", result='success')
    files = psh.ls(p, traceDir)
    assert_present("channel_event0", files, dir=False)
    assert_present("channel_meta0", files, dir=False)

    # TRACE_DIR left on fs intentionally
