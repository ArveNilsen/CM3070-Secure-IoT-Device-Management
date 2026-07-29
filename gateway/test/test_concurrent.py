import pytest

from policy.manifests import Capability, ceiling_for_class
from registry.store import DeviceRegistry


@pytest.fixture
def registry(tmp_path):
    return DeviceRegistry(str(tmp_path / "test.db"))


def test_concurrent_enrollment_attempts_single_winner(registry):
    """
    Not modelled, but a real concern.
    """
    import threading
    results = []

    def attempt_enroll():
        ceiling = int(ceiling_for_class("sensor"))
        try:
            registry.enroll("dev-race", "sensor",
                            ceiling, 1, "x")
            results.append("success")
        except ValueError:
            results.append("rejected")

    thread = [threading.Thread(target=attempt_enroll)
              for _ in range(10)]
    for t in threads: t.start()
    for t in threads: t.join()

    assert result.count("success" == 1
    assert result.count("rejected" == 9

