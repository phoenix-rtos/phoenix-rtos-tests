import pytest
import trunner

HARDCODED_VERB = 2


def _print(text: str):
    if HARDCODED_VERB >= 2:
        print(text)


@pytest.fixture(scope="session")
def conftest_print():
    return _print


@pytest.fixture(scope="session")
def pexpect_bin(dut):
    pexpect = dut.pexpect_proc
    pexpect.expect_exact("[Commence Fake Communication]")

    yield pexpect

    pexpect.sendline("EXIT")
    pexpect.expect_exact("[Success!]")


@pytest.fixture(scope="session")
def pexpect_psh(dut):
    pexpect = dut.pexpect_proc
    exec_cmd = "/bin/psh" if trunner.ctx.target.rootfs else "sysexec psh"
    pexpect.sendline(exec_cmd)
    pexpect.expect(rf"{exec_cmd}(\r+)\n")

    yield pexpect

    pexpect.send("\x04")
    pexpect.expect("exit(\r+)\n")
