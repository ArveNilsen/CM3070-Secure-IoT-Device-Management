from __future__ import annotations

from enum import IntFlag


class Capability(IntFlag):
    READ_SENSOR_DATA    = 1 << 0
    PUBLISH_TELEMETRY   = 1 << 1
    TRIGGER_ACTUATOR    = 1 << 2
    RECEIVE_CONFIG      = 1 << 3
    BROADCAST_PRESENCE  = 1 << 4


# ManifestWithinCeiling invariant
CLASS_CEILINGS: dict[str, Capability] = {
    "sensor": (
        Capability.READ_SENSOR_DATA |
        Capability.PUBLISH_TELEMETRY |
        Capability.BROADCAST_PRESENCE
    ),
    "actuator": (
        Capability.TRIGGER_ACTUATOR |
        Capability.RECEIVE_CONFIG |
        Capability.BROADCAST_PRESENCE
    ),
    "controller": (
        Capability.READ_SENSOR_DATA |
        Capability.PUBLISH_TELEMETRY |
        Capability.TRIGGER_ACTUATOR |
        Capability.RECEIVE_CONFIG |
        Capability.BROADCAST_PRESENCE
    ),
}

def ceiling_for_class(device_class: str) -> Capability:
    ceiling = CLASS_CEILINGS.get(device_class.lower())
    if ceiling is None:
        raise ValueError(f"Unknown device class: {device_class}")
    return ceiling
