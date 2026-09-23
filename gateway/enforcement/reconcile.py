"""
Re-applies per-device OVS flow state from the registry.
Run on startup.
"""
from registry.store import DeviceRegistry
from enforcement.ovs_backend import OVSBackend

def reconcile():
    registry = DeviceRegistry("registry.db")
    backend = OVSBackend()
    for device in registry.all_devices():
        if device.mac_adress is None:
            continue
        if device.state == "quarantined":
            backend.quarantine(device.mac_adress)
        elif device.state in ("enrolled", "restricted"):
            backend.apply_capabilities(device.mac_adress, device.active_caps)
    registry.close()


if __name__ == "__main__":
    reconcile()
