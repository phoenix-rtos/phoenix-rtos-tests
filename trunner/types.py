import re
import sys
import traceback
from dataclasses import dataclass, field
from typing import Callable, List, Optional

from trunner.text import bold, green, red, yellow


class TestResult:
    OK = "OK"
    FAIL = "FAIL"
    SKIP = "SKIP"

    def __init__(self, name=None, msg="", output="", status=None, verbosity=0):
        self.msg = msg
        self.status = TestResult.OK if status is None else status
        self.name = name
        self.output = output
        self.verbosity = verbosity

    def get_name(self) -> str:
        return self.name if self.name else "UNKNOWN TEST NAME"

    def __str__(self) -> str:
        result = []
        if self.is_ok():
            result.append(green("OK"))
        elif self.is_fail():
            result.append(red("FAIL"))
        elif self.is_skip():
            result.append(yellow("SKIP"))
        else:
            # Leaving it for debugging purpose
            result.append("UNKNOWN STATUS")

        result.append("\n")
        result.append(self.msg)
        if self.msg and self.msg[-1] != "\n":
            result.append("\n")

        if self.verbosity > 0 and self.output:
            result.append(bold("OUTPUT:\n"))
            result.append(self.output)
            if self.output[-1] != "\n":
                result.append("\n")

        return "".join(result)

    def is_fail(self):
        return self.status == TestResult.FAIL

    def is_skip(self):
        return self.status == TestResult.SKIP

    def is_ok(self):
        return self.status == TestResult.OK

    def fail(self, msg=None):
        if msg is not None:
            self.msg = msg

        self.status = TestResult.FAIL

    def skip(self):
        self.status = TestResult.SKIP

    def _failed_traceback(self) -> List[str]:
        _, _, exc_traceback = sys.exc_info()
        tb_info = traceback.format_tb(exc_traceback)[1:]  # Get rid off "self.harness()" call info
        return [bold("ASSERTION TRACEBACK (most recent call last):"), "".join(tb_info)]

    def _failed_before_buffer(self, dut) -> str:
        return dut.before.replace("\r", "")

    def _failed_buffer(self, dut) -> str:
        return dut.buffer.replace("\r", "")

    def fail_pexpect(self, dut, exc):
        self.fail()
        r_searched = r"[\d+]: (?:re.compile\()?b?['\"](.*)['\"]\)?"
        searched_patterns = re.findall(r_searched, exc.value)

        self.msg = "\n".join(
            [
                bold("EXPECTED:"),
                *(f"\t{idx}: {pattern}" for idx, pattern in enumerate(searched_patterns)),
                bold("GOT:"),
                self._failed_before_buffer(dut),
                "",
            ]
        )

    def fail_decode(self, dut, exc):
        self.fail()
        self.msg = "\n".join(
            [
                bold("Unicode Decode Error detected!"),
                *self._failed_traceback(),
                bold("STRING WITH NON-ASCII CHARACTER:"),
                str(exc.object),
                bold("OUTPUT CAUGHT BEFORE EXCEPTION:\n"),
                self._failed_before_buffer(dut),
                "",
            ]
        )

    def fail_assertion(self, dut, exc):
        self.fail()
        msg = [*self._failed_traceback()]

        if exc.args:
            msg.extend([bold("ASSERTION MESSAGE:"), str(exc)])

        if dut.buffer:
            msg.extend([bold("READER BUFFER:"), self._failed_buffer(dut)])

        msg.append("")
        self.msg = "\n".join(msg)

    def fail_unknown_exception(self):
        self.fail()
        self.msg = "\n".join([bold("EXCEPTION:"), traceback.format_exc()])


@dataclass
class AppOptions:
    file: str
    source: str = "usb0"
    imap: str = "ocram2"
    dmap: str = "ocram2"
    exec: bool = True


@dataclass
class BootloaderOptions:
    apps: List[AppOptions] = field(default_factory=list)


@dataclass
class ShellOptions:
    binary: Optional[str] = None
    cmd: Optional[List[str]] = None


@dataclass
class TestOptions:
    name: Optional[str] = None
    harness: Callable[[], Optional[TestResult]] = None
    target: Optional[str] = None
    bootloader: Optional[BootloaderOptions] = None
    shell: Optional[ShellOptions] = None
    should_reboot: bool = True
    ignore: bool = False
    nightly: bool = False
