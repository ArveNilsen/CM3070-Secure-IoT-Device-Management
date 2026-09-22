"""
Registers a provisioned device's public key and expected firmware hash in the
gateway's trust store.

Usage:
    python3 register_device devices/esp32-001.json
"""
import json
import sys
from pathlib import Path

_PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(_PROJECT_ROOT / "gateway"))

from registry.trust_store import TrustStore


def register(manifest_path: str):
    manifest = json.loads(Path(manifest_path).read_text())

    device_id = manifest["device_id"]
    device_class = manifest["device_class"]
    public_key_hex = manifest["public_key_hex"]
    firmware_hash_hex = manifest["firmware_hash_hex"]

    print(f"Registering device: {device_id}")
    print(f"    Class:  {device_class}")
    print(f"    Pubkey: {public_key_hex[:16]}...")

    confirm = input("Confirm registration? [y/N] ")
    if confirm.lower() != 'y':
        print("Aborted.")
        return

    store = TrustStore(str(_PROJECT_ROOT / "gateway" / "trust_store.db"))
    if store.is_registered(device_id):
        print(f"WARNING: '{device_id}' is already registered. "
              f"Re-run with a different device_id is this is a new device.")
        store.close()
        return

    store.register(device_id, device_class, public_key_hex, firmware_hash_hex)
    store.close()
    print("Registered.")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <manifest.json>")
        sys.exit(1)
    register(sys.argv[1])
