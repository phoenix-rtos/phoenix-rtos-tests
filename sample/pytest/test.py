import pytest


test_data = [
    ("ping", "[OK]"),
    ("echo", "[OK]"),
    ("abcd", "[FAIL]"),
]


def test_ctx(ctx):
    assert ctx is not None


def test_dut(dut):
    assert dut is not None


@pytest.mark.parametrize("cmd, exp", test_data)
def test_dut_parameterized_commands(pexpect_bin, cmd, exp):
    pexpect_bin.sendline(cmd)
    pexpect_bin.expect_exact(f"{exp}\r\n")


@pytest.mark.usefixtures("pexpect_bin")
class TestClass:
    test_val = 0

    def setup_method(self):
        self.__class__.test_val = 5

    def teardown_method(self):
        self.__class__.test_val = 0

    def test_send_hello_from_class(self, pexpect_bin):
        pexpect_bin.sendline("echo hello from class")
        pexpect_bin.expect_exact("\r\nhello from class\r\n")

    def test_test_val_value(self):
        assert self.__class__.test_val == 5


def test_class_static_val_is_zero():
    assert TestClass.test_val == 0
