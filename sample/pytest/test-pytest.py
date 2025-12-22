import pytest


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


def test_global_resource_ready_id_999(global_session_resource):
    assert global_session_resource["status"] == "ready"
    assert global_session_resource["id"] == 999

    global_session_resource["modified"] = True


def test_global_resource_already_modified(global_session_resource):
    assert global_session_resource.get("modified") is True
    assert global_session_resource["status"] == "ready"

@pytest.mark.xfail(strict=True)
def test_global_resource_not_busy(global_session_resource):
    assert global_session_resource["status"] == "busy"
