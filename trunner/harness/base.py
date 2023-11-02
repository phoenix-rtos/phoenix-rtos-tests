import time
from abc import ABC, abstractmethod
from typing import Callable, Dict, Optional, List

from trunner.text import bold
from trunner.host import Host
from trunner.dut import Dut
from trunner.types import TestResult, TestStage, is_github_actions


class HarnessError(Exception):
    """Base class for errors thrown in harnesses."""

    def __init__(self, msg: str = ""):
        self.msg = msg
        self.additional_info: Dict[str, str] = {}

    def add_additional_info(self, header: str, output: str):
        self.additional_info[header] = output

    def _format_additional_info(self) -> List[str]:
        err = []

        if self.additional_info:
            err.append(bold("ADDITIONAL INFO:"))

            for header, output in self.additional_info.items():
                if is_github_actions():
                    err.append(f"::group::{header}")

                err.extend([bold(header.upper() + ":"), output])

                if is_github_actions():
                    err.append("::endgroup::")

        return err

    def __str__(self):
        msg = self.msg
        if not msg:
            msg = "[there is no message]"

        err = [bold("HARNESS ERROR: ") + msg]
        err.extend(self._format_additional_info())
        err.append("")
        return "\n".join(err)


class ProcessError(HarnessError):
    name: str = "PROCESS"

    def __init__(self, msg: str = "", output: Optional[str] = None):
        super().__init__(msg)
        self.msg = msg
        self.output = output

    def __str__(self) -> str:
        # TODO format github actions output
        err = [bold(f"{self.name} ERROR: ") + self.msg]

        if self.output is not None:
            err.extend([bold("OUTPUT:"), self.output])

        err.extend(self._format_additional_info())
        err.append("")
        return "\n".join(err)


class FlashError(ProcessError):
    name: str = "FLASH"


class Rebooter:
    """Class that provides all necessary methods needed for rebooting target device."""

    def __init__(self, host: Host, dut: Dut):
        self.host = host
        self.dut = dut

    def _reboot_soft(self):
        self.host.set_reset(0)
        time.sleep(0.05)
        self.dut.clear_buffer()
        self.host.set_reset(1)
        # delay needed for plo usb device to disappear
        time.sleep(0.25)

    def _reboot_hard(self):
        self.host.set_power(False)
        time.sleep(0.5)
        self.dut.clear_buffer()
        self.host.set_power(True)
        time.sleep(0.5)

    def _reboot_dut_gpio(self, hard):
        if hard:
            self._reboot_hard()
        else:
            self._reboot_soft()

    def _reboot_dut_text(self, hard):
        pass

    def _set_flash_mode(self, flash):
        pass

    def __call__(self, flash=False, hard=False):
        """Sets flash mode and perform hard or soft reboot based on `hard` flag."""

        self._set_flash_mode(flash)

        if self.host.has_gpio():
            self._reboot_dut_gpio(hard=hard)
        else:
            # Perform rebooting with user interaction
            self._reboot_dut_text(hard=hard)


class HarnessBase(ABC):
    """Base class for harnesses functors that are tied together."""

    @abstractmethod
    def __call__(self, result: TestResult) -> TestResult:
        """Implements the harness logic. Every class that inherites must to implement this method.

        Parameters:
            result (TestResult): test result stub/partial result to be updated by the harness
        """


class TerminalHarness(HarnessBase):
    pass


class IntermediateHarness(HarnessBase):
    def __init__(self):
        self.next_harness = VoidHarness()

    def chain(self, harness: Callable[[TestResult], TestResult]):
        """Chains harnesses together in self -> harness order. Returns passed argument."""
        self.next_harness = harness
        return self.next_harness


class HarnessBuilder:
    """Class that builds a single-linked list abstraction over harness chain method."""

    def __init__(self):
        self.head = None
        self.tail = None

    def add(self, harness: Callable[[TestResult], TestResult]):
        """Add harness to the tail of list."""
        if self.tail is None:
            self.tail = harness
            self.head = harness
        else:
            self.tail = self.tail.chain(harness)

    def get_harness(self) -> Callable[[TestResult], TestResult]:
        """Returns the first harness in list."""
        if self.head is None:
            raise ValueError("Harness chain is empty!")

        return self.head


class VoidHarness(TerminalHarness):
    """Harness that does nothing."""

    def __call__(self, result: TestResult):
        return result


class RebooterHarness(IntermediateHarness):
    """Special harness to perform reboot of the device."""

    def __init__(self, rebooter, flash: bool = False, hard: bool = False):
        super().__init__()
        self.rebooter = rebooter
        self.flash = flash
        self.hard = hard

    def __call__(self, result: TestResult) -> TestResult:
        """Call rebooter with set flags and continue to execute the next harness."""

        result.set_stage(TestStage.REBOOT)
        self.rebooter(flash=self.flash, hard=self.hard)
        return self.next_harness(result)


class TestStartRunningHarness(IntermediateHarness):
    """Harness for explicitly stating the place the test has started running"""

    def __call__(self, result: TestResult) -> TestResult:
        result.set_stage(TestStage.RUN)
        return self.next_harness(result)
