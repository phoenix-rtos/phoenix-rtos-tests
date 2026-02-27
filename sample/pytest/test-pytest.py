import pytest
import time


@pytest.fixture(scope="session")
def global_session_resource():
    resource = {"status": "ready", "id": 999}

    yield resource

    resource["status"] = "closed"


@pytest.fixture(scope="function")
def counter():
    counter = {"count": 0}

    yield counter


test_data = [
    ("ping", "[OK]"),
    ("echo", "[OK]"),
    ("abcd", "[FAIL]"),
]


def test_always_passes():
    assert True


@pytest.mark.skip(reason="#x issue")
def test_always_skip():
    assert False


@pytest.mark.xfail(reason="#x issue")
def test_always_xfail():
    assert False


@pytest.mark.xfail()
def test_always_xpass():
    assert True


def test_ctx(ctx):
    assert ctx is not None


def test_dut(dut):
    assert dut is not None


def test_func_scope_counter_1(counter):
    assert counter.get("count") == 0
    counter["count"] += 1
    assert counter.get("count") == 1
    counter["count"] += 1
    assert counter.get("count") == 2


def test_func_scope_counter_2(counter):
    assert counter.get("count") == 0


def test_global_resource_ready_id_999(global_session_resource):
    assert global_session_resource["status"] == "ready"
    assert global_session_resource["id"] == 999


def test_subresult_time_by_delaying():
    time.sleep(0.5)
    assert True


@pytest.mark.parametrize("cmd, exp", test_data)
def test_dut_parameterized_commands(pexpect_bin, cmd, exp):
    pexpect_bin.sendline(cmd)
    pexpect_bin.expect_exact(f"{exp}\r\n")


@pytest.mark.usefixtures("pexpect_bin")
class TestClass:
    test_val = 0

    def setup_method(self):
        TestClass.test_val = 5

    def teardown_method(self):
        TestClass.test_val = 0

    def test_send_hello_from_class(self, pexpect_bin):
        pexpect_bin.sendline("hello from class")
        pexpect_bin.expect_exact("hello!\r\n")

    def test_test_val_value(self):
        assert TestClass.test_val == 5


def test_class_static_val_is_zero():
    assert TestClass.test_val == 0
