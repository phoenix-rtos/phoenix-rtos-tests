import pytest
import time


@pytest.fixture(scope="session")
def global_session_resource():
    resource = {"status": "ready", "id": 999}
    print("\n\t[SESSION] :::: STARTING\n")
    
    yield resource

    print("\n\t[SESSION] :::: TEARING DOWN\n")
    resource["status"] = "closed"


@pytest.fixture(scope="function")
def counter():
    counter = {"count": 0}

    yield counter


test_data = [
    ("Hello World", "OK"),
    ("ping", "OK"),
    ("exit", "OK"),
]


def test_always_passes():
    assert True


@pytest.mark.skip(reason="CI CHECK")
def test_always_fails(ctx):
    assert False, "Always Fail"


@pytest.mark.skip(reason="Test")
def test_always_skip():
    assert True


def test_always_xfail():
    pytest.xfail("xfail")


@pytest.mark.xfail(reason="always xfail")
def test_always_xpass():
    pass


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


def test_fake_dut_send_time(fake_dut_session, conftest_delay):
    exp_sleep_time = conftest_delay
    start = time.time()

    fake_dut_session.send(f"Hello")

    elapsed = time.time() - start

    assert elapsed >= exp_sleep_time


@pytest.mark.parametrize("cmd, exp_resp", test_data)
def test_dut_parameterized_commands(fake_dut_session, cmd, exp_resp, conftest_delay):
    start = time.time()
    response = fake_dut_session.send(cmd)
    elapsed = time.time() - start
    assert response == exp_resp
    assert elapsed <= (conftest_delay * 1.1)


@pytest.mark.usefixtures("fake_dut_class")
class TestClassScopeFixture:
    def test_no_ok_in_log(self, fake_dut_class):
        assert "OK" not in fake_dut_class.get_log()

    def test_send_hello(self, fake_dut_class):
        fake_dut_class.send("hello")
        assert "OK" in fake_dut_class.get_log()

    def test_send_world_check_for_2_OKs(self, fake_dut_class):
        fake_dut_class.send("hello")
        assert fake_dut_class.get_log().count("OK") == 2
