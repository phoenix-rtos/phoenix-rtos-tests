from __future__ import annotations
from datetime import datetime
from functools import total_ordering

import os
import re
import sys
import time
import traceback
import junitparser
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Callable, Dict, List, Optional
from pathlib import Path
from trunner.text import bold, green, red, remove_ansi_sequences, yellow


def is_github_actions() -> bool:
    return "GITHUB_ACTIONS" in os.environ


def get_ci_url() -> str:
    if is_github_actions():
        return (f"{os.environ['GITHUB_SERVER_URL']}/{os.environ['GITHUB_REPOSITORY']}"
                f"/actions/runs/{os.environ['GITHUB_RUN_ID']}")

    return ""


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
        if s in ("SKIP", "SKIPPED", "IGNORE", "IGNORED", "UNTESTED"):
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

    def to_junit(self, msg: str):
        assert self != Status.OK
        if self == Status.SKIP:
            return junitparser.Skipped(msg)

        return junitparser.Failure(msg)

    def should_print(self, verbosity: int):
        status_for_verbosity = (Status.FAIL, Status.SKIP, Status.OK)
        return self in status_for_verbosity[:verbosity + 1]


@total_ordering
class TestStage(Enum):
    """Testing stage to timed"""
    INIT = auto()
    REBOOT = auto()
    FLASH = auto()
    RUN = auto()
    DONE = auto()

    @staticmethod
    def important():
        """stages which are worth to be timed"""
        return [TestStage.REBOOT, TestStage.FLASH, TestStage.RUN]

    def __lt__(self, other):
        if self.__class__ is other.__class__:
            return self.value < other.value
        return NotImplemented


class TestResult:
    def __init__(self, name=None, msg: str = "", status: Optional[Status] = None):
        self.msg = msg
        self.summary = ""
        self.status = Status.OK if status is None else status
        self._name = name
        self.subname = ""

        # test execution tracking
        self._timing_stage: Optional[TestStage] = None
        self._timing_stage_start: float = 0
        self._timing_data: Dict[TestStage, float] = {}
        self._start_time = None
        self.set_stage(TestStage.INIT)

        # subresults
        self._curr_subresult: TestResult
        self.subresults: List[TestResult] = []

    @property
    def name(self) -> str:
        return self._name if self._name else "UNKNOWN TEST NAME"

    @property
    def full_name(self) -> str:
        """full canonical name of (sub)test"""
        out = self.name
        if self.subname:
            out += f".{self.subname}"
        return out

    @property
    def shortname(self) -> str:
        return self.name.replace("phoenix-rtos-tests/", "").replace("/", ".")

    def __str__(self) -> str:
        return self.to_str()

    def to_str(self, verbosity: int = 0) -> str:
        out = [str(self.status)]
        if self.msg:
            out.append(self.msg.rstrip())

        for res in self.subresults:
            if not res.status.should_print(verbosity):
                continue

            out.append(str(res))

        out.append("")
        return "\n".join(out)

    def to_junit_testcase(self, target: str):
        out = junitparser.TestCase(f"{target}:{self.full_name}")
        out.classname = self.name
        out.time = round(self._timing_data.get(TestStage.RUN, 0), 3)
        if self.status != Status.OK:
            # remove ANSI codes (not valid within XML)
            msg = remove_ansi_sequences(self.msg)
            summary = remove_ansi_sequences(self.summary)
            if not summary:
                # put first line as a failure reason
                summary = msg.splitlines()[0] if msg else ""
            out.result = [self.status.to_junit(summary)]
            if msg and msg != summary:
                # put detailed multi-line message as a system-out only if it brings new information
                out.system_out = msg

        return out

    def to_junit_testsuite(self, target: str):
        """return test result in junit format"""
        out = junitparser.TestSuite(f"{target}:{self.name}")
        if self._start_time:
            out.timestamp = self._start_time.isoformat()
        if not self.subresults:
            out.add_testcase(self.to_junit_testcase(target))
        else:
            for res in self.subresults:
                out.add_testcase(res.to_junit_testcase(target))

        return out

    def overwrite(self, other: TestResult):
        """Overwrite current global result (status, msg) with other one. Don't touch subtests"""
        self.msg = other.msg
        self.status = other.status
        self.set_stage(TestStage.DONE)

    def set_stage(self, stage: TestStage):
        assert self._timing_stage is None or stage >= self._timing_stage
        if stage == self._timing_stage:
            return

        if self._timing_stage is not None:
            duration = time.time() - self._timing_stage_start
            self._timing_data[self._timing_stage] = duration

        self._timing_stage = stage
        self._timing_stage_start = time.time()

        if stage == TestStage.RUN:
            self._start_time = datetime.utcnow()
            self._init_subresult()

    def _init_subresult(self):
        self._curr_subresult = TestSubResult(self.name)

    def _commit_subresult(self):
        self.subresults.append(self._curr_subresult)
        self._init_subresult()

    def add_subresult(self, subname: str, status: Status, msg: str = ""):
        """Add sub-testcase result (use for composite tests).

        Returns subresult object which can be modified at a later time.
        """
        subresult = self._curr_subresult
        subresult.set_stage(TestStage.DONE)
        subresult.msg = msg
        subresult.status = status
        subresult.subname = subname

        self._commit_subresult()
        return subresult

    def is_fail(self):
        return self.status == Status.FAIL

    def is_skip(self):
        return self.status == Status.SKIP

    def is_ok(self):
        return self.status == Status.OK

    def fail(self, msg=None, summary=None):
        if msg is not None:
            self.msg = msg

        if summary is not None:
            self.summary = summary

        self.status = Status.FAIL
        self.set_stage(TestStage.DONE)

    def skip(self):
        self.status = Status.SKIP
        self.set_stage(TestStage.DONE)

    def _failed_traceback(self) -> List[str]:
        _, _, exc_traceback = sys.exc_info()
        tb_info = traceback.format_tb(exc_traceback)[1:]  # Get rid off "self.harness()" call info
        return [bold("ASSERTION TRACEBACK (most recent call last):"), "".join(tb_info)]

    def _failed_before_buffer(self, dut) -> str:
        return dut.before.replace("\r", "")

    def _failed_buffer(self, dut) -> str:
        return dut.buffer.replace("\r", "")

    def fail_pexpect(self, dut, exc):
        self.fail(summary="Pexpect failure")
        r_searched = r"[\d+]: (?:re.compile\()?b?['\"](.*)['\"]\)?"
        searched_patterns = re.findall(r_searched, exc.value)

        self.msg = "\n".join(
            [
                bold("EXPECTED:"),
                *(f"\t{idx}: {pattern}" for idx, pattern in enumerate(searched_patterns)),
                bold("GOT:"),
                self._failed_before_buffer(dut),
                *self._failed_traceback(),
                "",
            ]
        )

        # if composite test case - fail the sub-result
        if self.subresults:
            self._curr_subresult.fail_pexpect(dut, exc)
            self._commit_subresult()

    def fail_decode(self, dut, exc):
        self.fail(summary="Unicode Decode Error")
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
        self.fail(summary="Assertion failed")
        msg = [*self._failed_traceback()]

        if exc.args:
            msg.extend([bold("ASSERTION MESSAGE:"), str(exc)])

        if dut.buffer:
            msg.extend([bold("READER BUFFER:"), self._failed_buffer(dut)])

        msg.append("")
        self.msg = "\n".join(msg)

        # if composite test case - fail the sub-result
        if self.subresults:
            self._curr_subresult.fail_assertion(dut, exc)
            self._commit_subresult()

    def fail_harness_exception(self, exc):
        self.fail(summary="Harness Exception")
        self.msg = "\n".join([*self._failed_traceback(), str(exc)])

    def fail_unknown_exception(self):
        self.fail(summary="Unknown Exception")
        self.msg = "\n".join([bold("EXCEPTION:"), traceback.format_exc()])

    @staticmethod
    def get_csv_header() -> str:
        data = ["name", "subname", "status"]
        data.extend([str(stage) for stage in TestStage.important()])

        return ",".join(data)

    def to_csv(self) -> str:
        data = [self.name, self.subname, self.status.name]
        data.extend([f"{self._timing_data.get(stage, 0):.3f}" for stage in TestStage.important()])

        return "\n".join([",".join(data), *[subres.to_csv() for subres in self.subresults]])


class TestSubResult(TestResult):
    """Helper class that can be used to build more complex test results.

    It may be helpful in more complex harnesses that parses output from other
    testing framework to build output.

    Attributes:
        subname: Name of subtest
        status: Status of test
        msg: Message that will be printed after test header

    TestSubResult needs to be linked to parent TestResult by add_subresult or add_subresult_obj.
    """

    def __init__(self, name=None, subname: str = "", msg: str = "", status: Optional[Status] = None):
        super().__init__(name, msg, status)
        self.subname = subname
        self.set_stage(TestStage.RUN)

    def __str__(self) -> str:
        out = f"\t{bold(self.subname)}: {self.status}"

        if self.msg and self.status != Status.OK:
            out += ": " + self.msg

        return out

    def _init_subresult(self):
        # subresults can't contain subresults
        pass


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
    path: Optional[Path] = Path("bin")


def void_harness_fn(result: TestResult) -> TestResult:
    """dummy harness just to satisfy python typing"""
    return result


@dataclass
class TestOptions:
    name: Optional[str] = None
    harness: Callable[[TestResult], TestResult] = void_harness_fn
    target: Optional[str] = None
    bootloader: Optional[BootloaderOptions] = None
    shell: Optional[ShellOptions] = None
    should_reboot: bool = False
    ignore: bool = False
    nightly: bool = False
    kwargs: Dict = field(default_factory=dict)
