import pytest

from policy.manifests import Capability, ceiling_for_class
from registry.store import DeviceRegistry


@pytest.fixture
def registry(tmp_path):
    return DeviceRegistry(str(tmp_path / "test.db"))

def test_capability_ceiling_invariant(registry):
    """
    Formal property: CapabilityCeiling (TLA+)
    Active capabilities never exceed enrolled manifest.
    """
    registry.enroll("dev-1", "sensor",
                    capabilities=int(ceiling_for_class("sensor")),
                    manifest_version=1,
                    firmware_hash="abc")

    device = registry.get("dev-1")
    assert device.active_caps & ~device.capabilities == 0


def test_restrict_cannot_exceed_ceiling(registry):
    """
    Formal property: ActiveSubset (Alloy) / Restrict guard (TLA+)
    """
    ceiling = int(ceiling_for_class("sensor"))
    registry.enroll("dev-1", "sensor", ceiling, 1, "abc")

    # Attempt to restrict to a mask exceeding the ceiling
    excessive_mask = ceiling | Capability.TRIGGER_ACTUATOR
    registry.restrict("dev-1", excessive_mask)

    device = registry.get("dev-1")
    # The ceiling must win, intersection enforced
    assert device.active_caps == (excessive_mask & ceiling)
    assert device.active_caps & ~ceiling == 0


def test_no_double_enrollment(registry):
    """
    Formal property: NoDoubleEnrollment (Alloy)
    """
    ceiling = int(ceiling_for_class("sensor"))
    registry.enroll("dev-1", "sensor", ceiling, 1, "abc")

    with pytest.raises(ValueError):
        registry.enroll("dev-1", "sensor", ceiling, 2, "abc")


def test_quarantine_is_isolated(registry):
    """
    Formal property; QuarantineIsIsolated (TLA+)
    """
    ceiling = int(ceiling_for_class("sensor"))
    registry.enroll("dev-1", "sensor", ceiling, 1, "abc")
    registry.quarantine("dev-1")

    device = registry.get("dev-1")
    assert device.active_caps == 0
    assert device.state == "quarantined"


def test_revoked_has_no_manifest(registry):
    """
    Formal property: RevokedHasNoManifest (TLA+)
    """
    ceiling = int(ceiling_for_class("sensor"))
    registry.enroll("dev-1", "sensor", ceiling, 1, "abc")
    registry.revoke("dev-1")

    assert registry.is_enrolled("dev-1") is False


def test_quarantine_requires_revoke_before_reenroll(registry):
    """
    Formal design decision: Quarantined -> Unenroll requires an explicit
    Revoke step. This matches TLA+ model's ReenrollFromRevoked.
    """
    ceiling = int(ceiling_for_class("sensor"))
    registry.enroll("dev-1", "sensor", ceiling, 1, "abc")
    registry.quarantine("dev-1")

    # Attempting to re-enroll directly should fail. Device is still "enrolled"
    # in registry terms (state=quarantined, not revoked.
    with pytest.raises(ValueError):
        registry.enroll("dev-1", "sensor", ceiling, 2, "abc")

    # Correct path: revoke first
    registry.revoke("dev-1")
    registry.enroll("dev-1", "sensor", ceiling, 2, "abc")
    assert registry.is_enrolled("dev-1") is True
