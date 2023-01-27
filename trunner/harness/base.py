import time
from typing import Optional

from trunner.text import bold
from trunner.host import Host
from trunner.dut import Dut


class HarnessError(Exception):
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
        self._set_flash_mode(flash)

        if self.host.has_gpio():
            self._reboot_dut_gpio(hard=hard)
        else:
            # Perform rebooting with user interaction
            self._reboot_dut_text(hard=hard)


class HarnessBuilder:
    def __init__(self):
        self.head = VoidHarness()
        self.tail = self.head

    def chain(self, harness):
        self.tail = self.tail.chain(harness)

    def get_harness(self):
        return self.head.harness


class HarnessBase:
    def __init__(self):
        self.harness = lambda: None

    def __call__(self):
        raise NotImplementedError("__call__ should no be called from the base class")

    def chain(self, harness):
        self.harness = harness
        return self.harness


class VoidHarness(HarnessBase):
    def __call__(self):
        pass


class RebooterHarness(HarnessBase):
    def __init__(self, rebooter, flash=False, hard=False):
        self.rebooter = rebooter
        self.flash = flash
        self.hard = hard
        super().__init__()

    def __call__(self):
        self.rebooter(flash=self.flash, hard=self.hard)
        return self.harness()
