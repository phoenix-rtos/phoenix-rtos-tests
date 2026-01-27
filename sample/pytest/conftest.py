import pytest

HARDCODED_DELAY = 1
HARDCODED_VERB = 2


def _print(text: str):
    if HARDCODED_VERB >= 2:
        print(text)


@pytest.fixture(scope="session")
def conftest_delay():
    return HARDCODED_DELAY


@pytest.fixture(scope="session")
def conftest_print():
    return _print


@pytest.fixture(scope="session")
def pexpect(dut):
    pexpect = dut.pexpect_proc
    pexpect.expect_exact("[Commence Fake Communication]")

    yield pexpect

    pexpect.sendline("EXIT")
    pexpect.expect_exact("[Success!]")
