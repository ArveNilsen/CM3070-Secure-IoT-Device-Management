from hypothesis import strategies as st
from hypothesis.stateful import (
    RuleBasedStateMachine, rule, precondition,
    invariant, Bundle
)
from registry.store import DeviceRegistry
from policy.manifests import ceiling_for_class
import tempfile, os

class RegistryStateMachine(RuleBasedStateMachine):
    """
    Generates random sequences of enroll/restrict/quarantine/revoke/restore
    operations, and checks formal invariants hold after every step.
    """

    def __init__(self):
        super().__init__()
        self.db_path = tempfile.mktemp(suffix=".db")
        self.registry = DeviceRegistry(self.db_path)
        self.model_state = {} # shadow model TODO: Decide if needed.

    devices = Bundle("devices")

    @rule(target=devices,
        device_id=st.uuids().map(str),
        device_class=st.sampled_from(["sensor", "actuator", "controller"]))
    def enroll(self, device_id, device_class):
        if self.registry.is_enrolled(device_id):
            return # precondition violated, skip
        ceiling = int(ceiling_for_class(device_class))
        self.registry.enroll(device_id, device_class, ceiling, 
                             manifest_version=1, firmware_hash="x")
        return device_id

    @rule(device_id=devices, mask=st.integers(min_value=0, max_value=31))
    def restrict(self, device_id, mask):
        if not self.registry.is_enrolled(device_id):
            return
        try:
            self.registry.restrict(device_id, mask)
        except ValueError:
            pass # device revoked mid-sequence = fine

    @rule(device_id=devices)
    def quarantine(self, device_id):
        if not self.registry.is_enrolled(device_id):
            return
        try:
            self.registry.quarantine(device_id)
        except ValueError:
            pass

    @rule(device_id=devices)
    def restore(self, device_id):
        try:
            self.registry.restore(device_id)
        except ValueError:
            pass # expected: not restricted/revoked

    @rule(device_id=devices)
    def revoke(self, device_id):
        try:
            self.registry.revoke(device_id)
        except ValueError:
            pass

    # --- Invariants checked after every rule application ---

    @invariant()
    def capability_ceiling_never_exceeded(self):
        """CapabilityCeiling - TLA+"""
        for device in self.registry.all_devices():
            assert device.active_caps & ~device.capabilities == 0

    @invariant()
    def no_capability_without_enrollment(self):
        """NoCapabilityWithoutEnrollment - TLA+"""
        for device in self.registry.all_devices():
            if device.active_caps != 0:
                assert device.state in ("enrolled", "restricted")

    @invariant()
    def quarantine_is_isolated(self):
        """QuarantineIsIsolated - TLA+"""
        for device in self.registry.all_devices():
            if device.state == "quarantined":
                assert device.active_caps == 0

    @invariant()
    def revoked_has_no_manifest_semantics(self):
        """RevokedHasNoManifest - TLA+
        (registry represents 'no manifest' as
        active_caps == 0 and is_enrolled() == False"""
        for device in self.registry.all_devices():
            if device.state == "revoked":
                assert self.registry.is_enrolled(
                    device.public_key_id) is False

    def teardown(self):
        self.registry.close()
        os.remove(self.db_path)

TestRegistryStateMachine = RegistryStateMachine.TestCase
