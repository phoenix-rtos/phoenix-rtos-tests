import importlib
from abc import ABC, abstractmethod

from trunner.ctx import TestContext


class Host(ABC):  # pylint: disable=too-few-public-methods
    """Base class for Host abstraction"""
    name: str

    @abstractmethod
    def has_gpio(self) -> bool:
        """Return true if host has gpio utility and can restart the target device using it."""

    @classmethod
    @abstractmethod
    def from_context(cls, _):
        pass

    def add_to_context(self, ctx: TestContext) -> TestContext:
        """Host can use this method to add things to ctx or modify it. A new context is returned."""
        return ctx


class EmulatorHost(Host):
    """Host class for the targets that are intended to run on PC (for example, emulated targets)."""

    name = "pc"

    @classmethod
    def from_context(cls, _):
        return cls()

    def has_gpio(self) -> bool:
        return False


class GPIO:
    """Wrapper around the RPi.GPIO module. It represents a single OUT pin"""

    def __init__(self, pin, init=0):
        self.pin = pin
        self.gpio = importlib.import_module("RPi.GPIO")

        self.gpio.setmode(self.gpio.BCM)
        self.gpio.setwarnings(False)
        if init == 0:
            self.gpio.setup(self.pin, self.gpio.OUT, initial=self.gpio.LOW)
        else:
            self.gpio.setup(self.pin, self.gpio.OUT, initial=self.gpio.HIGH)

    def high(self):
        self.gpio.output(self.pin, self.gpio.HIGH)

    def low(self):
        self.gpio.output(self.pin, self.gpio.LOW)


class RpiHost(Host):
    """Host class for Raspberry Pi computer.

    Attributes:
        reset_gpio: GPIO to reset the board.
        power_gpio: GPIO to turn on/off power to the board.
        boot_gpio: GPIO to enter booting mode to program the board.
    """

    name = "rpi"

    def __init__(self):
        self.reset_gpio = GPIO(17, init=1)
        self.power_gpio = GPIO(2, init=1)
        self.boot_gpio = GPIO(4, init=0)

    @classmethod
    def from_context(cls, _):
        return cls()

    def set_reset(self, val):
        if val:
            self.reset_gpio.high()
        else:
            self.reset_gpio.low()

    def set_power(self, val):
        if val:
            self.power_gpio.high()
        else:
            self.power_gpio.low()

    def set_flash_mode(self, val):
        if val:
            self.boot_gpio.high()
        else:
            self.boot_gpio.low()

    def has_gpio(self):
        return True
