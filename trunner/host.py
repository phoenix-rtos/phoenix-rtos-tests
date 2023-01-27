import importlib


class Host:  # pylint: disable=too-few-public-methods
    def has_gpio(self) -> bool:
        raise NotImplementedError


class EmulatorHost(Host):
    name = "pc"

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
    name = "rpi"

    def __init__(self):
        self.reset_gpio = GPIO(17, init=1)
        self.power_gpio = GPIO(2, init=1)
        self.boot_gpio = GPIO(4, init=0)

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
        # TODO property
        return True
