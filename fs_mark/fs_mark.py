import re

from typing import Optional

import psh.tools.psh as psh

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status

from time_limits import time_limits


def _fs_clean(dut, ctx):
    if any(opt in ctx.cmd for opt in ("-k", "-L", "-F")):
        dirs = re.findall(r"-d\s+([^\s]+)", ctx.cmd)
        psh.send_cmd(dut, f"fs_mark_clean {' '.join(dirs)}", exec_prefix=True)


def harness(dut: Dut, ctx: TestContext) -> Optional[TestResult]:
    loop_line = r"(?P<line>(\s+\d+){3}.+?)\r+\n"
    end_msg = r"p99 Files/sec"
    error = r"fs_mark: (?P<err_msg>.+?)\r+\n"

    headers = ("FSUse%", "Count", "Size", "Files/sec", "App Overhead",
               "creatMin", "creatAvg", "creatMax",
               "writeMin", "writeAvg", "writeMax",
               "fsyncMin", "fsyncAvg", "fsyncMax",
               "syncMin", "syncAvg", "syncMax",
               "closeMin", "closeAvg", "closeMax",
               "unlinkMin", "unlinkAvg", "unlinkMax")

    limits = time_limits[ctx.target.name]

    while True:
        idx = dut.expect([loop_line, error, end_msg], 600)
        parsed = dut.match.groupdict()

        if idx == 0:
            figures = dict(zip(headers, parsed["line"].split()))
            for name, value in list(figures.items())[5:]:
                lower_bound = limits.get(name)[0]
                upper_bound = limits.get(name)[1]

                if not lower_bound <= int(value) <= upper_bound:
                    msg = f"{name}: {value} not in range ({lower_bound},{upper_bound})"
                    return TestResult(msg=msg, status=Status.FAIL)

        elif idx == 1:
            return TestResult(msg=parsed["err_msg"], status=Status.FAIL)

        elif idx == 2:
            _fs_clean(dut, ctx)
            return TestResult(status=Status.OK)
