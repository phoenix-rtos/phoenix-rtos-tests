import re
from pathlib import Path
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult


def normalize_plain_output_to_regex(text: str) -> str:
    """
    Convert plain expected output into strict regex matching CRLF/LF differences.
    """
    lines = text.splitlines()
    expected = r"\r?\n".join(re.escape(line) for line in lines)

    if text.endswith("\n"):
        expected += r"\r?\n"

    return expected


def harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs):
    test_name = kwargs.get("test", None)
    file = Path(result.name).name.replace(".", "/")

    tests_path = (Path(__file__).resolve().parent / f"../../_fs/{ctx.target.name}/root/usr/test/micropython").resolve()
    exp_file = tests_path / f"{file}.py.exp"
    expected_output = exp_file.read_text(encoding="utf-8")

    expected = normalize_plain_output_to_regex(expected_output)

    command = f"sysexec micropython syspage/{test_name}"
    skip_pattern = rf"{re.escape(command)}\r?\nSKIP"

    dut.sendline(command)
    idx = dut.expect([expected, skip_pattern], timeout=100)

    if idx == 0:
        return TestResult(status=Status.OK)
    if idx == 1:
        return TestResult(status=Status.SKIP)

    return TestResult(status=Status.FAIL)
