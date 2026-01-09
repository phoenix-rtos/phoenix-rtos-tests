import pytest
import time

HARDCODED_DELAY = 1
HARDCODED_VERB = 0


def _print(text: str):
    if HARDCODED_VERB >= 2:
        print(text)


class FakeDUT:
    def __init__(self):
        self._log = ""
        self.__printout("Device Initialized")
        self.connected = True

    def send(self, command: str) -> str:
        self.__printout(f"Sending {command}...")
        self.go_sleep(HARDCODED_DELAY)
        response = "OK"
        self.__printout(response)
        return response

    def go_sleep(self, seconds: int):
        time.sleep(seconds)

    def power_off(self):
        self.__printout("Powering OFF...")
        self.go_sleep(HARDCODED_DELAY)
        self.connected = False

    def get_log(self):
        return self._log

    def __printout(self, text: str):
        output = f"\n\t[FakeDUT] >> {text}"
        self._log += output
        _print(output)


@pytest.fixture(scope="session")
def conftest_delay():
    return HARDCODED_DELAY


@pytest.fixture(scope="session")
def conftest_print():
    return _print


@pytest.fixture(scope="session")
def fake_dut_session():
    dut = FakeDUT()

    yield dut

    dut.power_off()


@pytest.fixture(scope="class")
def fake_dut_class():
    dut = FakeDUT()

    yield dut

    dut.power_off()
