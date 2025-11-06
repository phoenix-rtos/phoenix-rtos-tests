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
