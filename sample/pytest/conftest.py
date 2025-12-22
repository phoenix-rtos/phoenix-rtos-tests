import pytest


@pytest.fixture(scope="session")
def pexpect_bin(dut):
    p = dut.pexpect_proc
    p.expect_exact("[Commence Fake Communication]\r\n")

    yield p

    p.sendline("EXIT")
    p.expect_exact("[Success!]")


@pytest.fixture(scope="session")
def pexpect_psh(dut, ctx):
    p = dut.pexpect_proc
    exec_cmd = "/bin/psh" if ctx.target.rootfs else "sysexec psh"
    p.sendline(exec_cmd)
    p.expect(rf"{exec_cmd}(\r+)\n")

    yield p

    p.send("\x04")
    p.expect("exit(\r+)\n")
