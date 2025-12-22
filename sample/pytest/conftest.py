import pytest

@pytest.fixture(scope="session")
def global_session_resource():
    resource = {"status": "ready", "id": 999}
    print("\n\t[SESSION] :::: STARTING\n")
    
    yield resource

    print("\n\t[SESSION] :::: TEARING DOWN\n")
    resource["status"] = "closed"
