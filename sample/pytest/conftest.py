import pytest
import trunner


@pytest.fixture(scope="session")
def pexpect_bin(dut):
    p = dut.pexpect_proc

    yield p

    p.sendline("EXIT")
    p.expect_exact("[Success!]")


@pytest.fixture(scope="session")
def pexpect_psh(dut):
    p = dut.pexpect_proc
    exec_cmd = "/bin/psh" if trunner.ctx.target.rootfs else "sysexec psh"
    p.sendline(exec_cmd)
    p.expect(rf"{exec_cmd}(\r+)\n")

    yield p

    p.send("\x04")
    p.expect("exit(\r+)\n")
