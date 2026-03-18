import re
from pathlib import Path
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult

import psh.tools.psh as psh

TESTS_WITH_REGEX_OUTPUT = {
    "micropython/meminfo.py",
    "basics/bytes_compare3.py",
    "basics/builtin_help.py",
    "thread/thread_exc2.py",
    "esp32/partition_ota.py",
    "cmdline/cmd_parsetree.py",
    "cmdline/cmd_showbc.py",
    "cmdline/cmd_verbose.py",
    "cmdline/cmd_showbc_const.py",
    "cmdline/cmd_showbc_opt.py",
}

MICROPYTHON_ROOT = (
    Path(__file__).resolve().parent
    / "../../_fs/armv7r5f-zynqmp-qemu/root/usr/test/micropython"
).resolve()

def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")

def _open_editor(dut: Dut, test_name: str, attempts: int = 30):
    dut.sendline(f"edit {test_name}")

    # Hack
    for _ in range(attempts):
        dut.sendline("")


def _save_editor_content(dut: Dut, content: str):
    dut.send(content)
    dut.send("\x13")  # Ctrl+S
    dut.send("\x11")  # Ctrl+Q

def harness(dut: Dut, ctx: TestContext, result: TestResult, **kwargs):
    test_name =  kwargs.get("test", None)

    test_file = MICROPYTHON_ROOT / test_name
    exp_file = MICROPYTHON_ROOT / f"{test_name}.exp"

    test_content = _read_text(test_file)
    expected_output = _read_text(exp_file)

    test_path = Path(test_name)
    if test_path.parent != Path("."):
        psh.assert_cmd(dut, f"mkdir {test_path.parent}")

    _open_editor(dut, test_name)
    _save_editor_content(dut, test_content)

    psh.assert_prompt(dut, timeout=60)

    command = f"sysexec micropython {test_name}"
    skip_pattern = rf"{re.escape(command)}\r?\nSKIP"

    if test_name not in TESTS_WITH_REGEX_OUTPUT:
        expected = re.escape(expected_output).replace("\n", "\r?\n")
    else:
        expected = test_name

    dut.sendline(command)
    idx = dut.expect([expected, skip_pattern], timeout=100)

    if idx == 0:
        return TestResult(status=Status.OK)
    if idx == 1:
        return TestResult(status=Status.SKIP)

    return TestResult(status=Status.FAIL)
