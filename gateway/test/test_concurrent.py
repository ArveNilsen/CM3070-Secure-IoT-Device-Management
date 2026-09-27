import pytest
import threading
import sqlite3

import sys
from pathlib import Path

_PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(_PROJECT_ROOT))

from policy.manifests import ceiling_for_class
from registry.store import DeviceRegistry


@pytest.fixture
def registry(tmp_path):
    return DeviceRegistry(str(tmp_path / "test.db"))


def test_concurrent_enrollment_attempts_single_winner(registry):
    """
    Evidences UniqueHardwareIdentityPerDevice under concurrent access.
    """
    results = []
    lock = threading.Lock()

    def attempt_enroll():
        ceiling = int(ceiling_for_class("sensor"))
        try:
            registry.enroll(
                public_key_id="dev-race", 
                device_class="sensor",
                capabilities=ceiling,
                manifesti_version=1, 
                firmware_hash="a" * 64)
            outcome = "success"
        except (ValueError, sqlite3.IntegrityError):
            outcome = "rejected"
        with lock:
            results.append(outcome)

    threads = [threading.Thread(target=attempt_enroll)
              for _ in range(10)]
    for t in threads: 
        t.start()
    for t in threads: 
        t.join()

    assert results.count("success") == 1
    assert results.count("rejected") == 9

