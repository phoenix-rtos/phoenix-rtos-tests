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


def harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs) -> TestResult:
    test_name = result.name.split(".")[-1] + ".py"
    base_subpath = result.name.split("/")[-1].replace(".", "/")

    # test files are currently taken directly from the host due to the following issues:
    # phoenix-rtos/phoenix-rtos-project#1600
    # phoenix-rtos/phoenix-rtos-project#1599
    tests_path = (Path(__file__).parents[2] / f"_fs/{ctx.target.name}/root/usr/test/micropython").resolve()
    exp_file_path = tests_path / f"{base_subpath}.py.exp"
    expected_output = exp_file_path.read_text(encoding="utf-8")

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
