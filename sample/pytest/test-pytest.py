import pytest
import time


@pytest.fixture(scope="session")
def global_session_resource(conftest_print):
    resource = {"status": "ready", "id": 999}
    conftest_print("\n\t[SESSION] :::: STARTING\n")

    yield resource

    conftest_print("\n\t[SESSION] :::: TEARING DOWN\n")
    resource["status"] = "closed"


@pytest.fixture(scope="function")
def counter():
    counter = {"count": 0}

    yield counter


test_data = [
    ("ping", "[Ping Complete]"),
    ("echo", "[OK]"),
    ("abcd", "[OK]"),
]


def test_always_passes():
    assert True


@pytest.mark.skip(reason="CI CHECK")
def test_always_fails():
    assert False, "Always Fail"


@pytest.mark.skip(reason="Should not fail because it's skipped")
def test_always_skip():
    assert False


@pytest.mark.xfail(reason="Always fails, as expected")
def test_always_xfail():
    assert False


@pytest.mark.xfail(reason="Always passes but it should not")
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

    global_session_resource["modified"] = True


def test_global_resource_already_modified(global_session_resource):
    assert global_session_resource.get("modified") is True
    assert global_session_resource["status"] == "ready"


@pytest.mark.xfail(strict=True)
def test_global_resource_not_busy_xfail(global_session_resource):
    assert global_session_resource["status"] == "busy"


def test_subresult_time_by_delaying():
    time.sleep(2)
    assert True


def test_send_hello(pexpect_bin):
    pexpect_bin.sendline("hello")
    pexpect_bin.expect_exact("hello [OK]")


@pytest.mark.parametrize("msg, exp", test_data)
def test_dut_parameterized_commands(pexpect_bin, msg, exp):
    pexpect_bin.sendline(msg)
    pexpect_bin.expect_exact(exp)


@pytest.mark.usefixtures("pexpect_bin")
class TestClass:
    test_val = 0

    def setup_method(self):
        TestClass.test_val = 5

    def teardown_method(self):
        TestClass.test_val = 0

    def test_send_hello_from_class(self, pexpect_bin):
        pexpect_bin.sendline("hello from class")
        pexpect_bin.expect_exact("[OK]")

    def test_test_val_value(self):
        assert TestClass.test_val == 5


def test_class_static_val_is_zero():
    assert TestClass.test_val == 0
