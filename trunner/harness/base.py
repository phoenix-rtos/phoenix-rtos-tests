import time
from typing import Optional

from trunner.text import bold
from trunner.host import Host
from trunner.dut import Dut


class HarnessError(Exception):
    """Base class for errors thrown in harnesses."""

    pass


class FlashError(HarnessError):
    def __init__(self, msg: Optional[str] = None, output: Optional[str] = None):
        self.msg = msg
        self.output = output

    def __str__(self):
        err = bold("FLASH ERROR: ") + (self.msg if self.msg else "") + "\n"

        if self.output is not None:
            err += bold("OUTPUT:") + "\n" + self.output + "\n"

        return err


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


class HarnessBuilder:
    """Class that builds a single-linked list abstraction over harness chain method."""

    def __init__(self):
        # We use VoidHarness as dummy-head of the list
        self.head = VoidHarness()
        self.tail = self.head

    def chain(self, harness):
        """Add harness to the tail of list."""
        self.tail = self.tail.chain(harness)

    def get_harness(self):
        """Returns the first harness in list."""
        return self.head.harness


class HarnessBase:
    """Base class for harnesses functors that are tied together."""

    def __init__(self):
        # TODO Here we should rather set it to the None value, now we can't check from the class
        # if we are the last harness in the chain.
        self.harness = lambda: None

    def __call__(self):
        """Implements the harness logic. Every class that inherites must to implement this method."""
        raise NotImplementedError("__call__ should no be called from the base class")

    def chain(self, harness):
        """Chains harnesses together in self -> harness order. Returns passed argument."""
        self.harness = harness
        return self.harness


class VoidHarness(HarnessBase):
    """Harness that does nothing."""

    def __call__(self):
        pass


class RebooterHarness(HarnessBase):
    """Special harness to perform reboot of the device."""

    def __init__(self, rebooter: Rebooter, flash: bool = False, hard: bool = False):
        self.rebooter = rebooter
        self.flash = flash
        self.hard = hard
        super().__init__()

    def __call__(self):
        """Call rebooter with set flags and continue to execute the next harness."""

        self.rebooter(flash=self.flash, hard=self.hard)
        return self.harness()
