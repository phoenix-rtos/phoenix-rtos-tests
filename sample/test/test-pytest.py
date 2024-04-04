import pytest


def test_always_passes():
    assert True


@pytest.mark.skip(reason="CI CHECK")
def test_always_fails():
    assert False


@pytest.mark.skip(reason="Test")
def test_always_skip():
    assert True


def test_always_xfail():
    pytest.xfail("xfail")


@pytest.mark.xfail(reason="always xfail")
def test_always_xpass():
    pass
