from __future__ import annotations

import os
import re
import sys
import traceback
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Callable, List, Optional, Sequence

import trunner
from trunner.text import bold, green, red, yellow


def is_github_actions() -> bool:
    return "GITHUB_ACTIONS" in os.environ


class Status(Enum):
    OK = auto()
    FAIL = auto()
    SKIP = auto()

    @classmethod
    def from_str(cls, s):
        if s in ("FAIL", "FAILED", "BAD"):
            return Status.FAIL
        if s in ("OK", "PASS", "PASSED"):
            return Status.OK
        if s in ("SKIP", "SKIPPED", "IGNORE", "IGNORED"):
            return Status.SKIP

        raise ValueError(f"Cannot create {cls.__name__} from string {s}")

    def color(self) -> Callable[[str], str]:
        if self == Status.OK:
            return green
        elif self == Status.FAIL:
            return red
        else:
            return yellow

    def __str__(self) -> str:
        color = self.color()
        return color(self.name)


class TestResult:
    def __init__(self, name=None, msg="", output="", status=None, verbosity=0):
        self.msg = msg
        self.status = Status.OK if status is None else status
        self.name = name
        self.output = output
        self.verbosity = verbosity

    def get_name(self) -> str:
        return self.name if self.name else "UNKNOWN TEST NAME"

    def __str__(self) -> str:
        result = [str(self.status)]
        if self.msg:
            result.append(self.msg.rstrip())

        if self.output and self.verbosity > 0:
            result.extend([bold("OUTPUT:"), self.output.rstrip()])

        result.append("")
        return "\n".join(result)

    def is_fail(self):
        return self.status == Status.FAIL

    def is_skip(self):
        return self.status == Status.SKIP

    def is_ok(self):
        return self.status == Status.OK

    def fail(self, msg=None):
        if msg is not None:
            self.msg = msg

        self.status = Status.FAIL

    def skip(self):
        self.status = Status.SKIP

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
class Result:
    """Helper class that can be used to build more complex test results.

    It may be helpful in more complex harnesses that parses output from other
    testing framework to build output.

    Attributes:
        name: Name of test/subtest
        status: Status of test
        msg: Message that will be printed after test header
    """

    name: str
    status: Status = Status.OK
    msg: str = ""

    @staticmethod
    def format_output(results: Sequence[Result]) -> str:
        output = []

        for res in results:
            if res.status != Status.FAIL and trunner.ctx.verbosity == 0:
                continue

            output.append(f"\t{bold(res.name)}: {res.status}")
            if res.msg and res.status == Status.FAIL:
                output.append(res.msg)

        output.append("")
        return "\n".join(output)


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
