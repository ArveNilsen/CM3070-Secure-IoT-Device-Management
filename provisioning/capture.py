"""
Captures provisioning output from ESP32 serial console and writes a device
manifest file. Start this script before executing the provisioning firmware
on the device.

Usage:
    python3 capture.py --port /dev/ttyUSB0 \
                       --device-id esp32-001 \
                       --device-class sensor
"""
import argparse
import json
import time
from datetime import datetime, timezone
from pathlib import Path

import serial

READ_TIMEOUT_S = 30



def capture(port: str, device_id: str, device_class: str):
    ser = serial.Serial(port, 115200, timeout=2)
    print(f"Listening on {port} for provisioning output...")

    data = None
    deadline = time.monotonic() + READ_TIMEOUT_S
    while time.monotonic() < deadline:
        line = ser.readline().decode(errors='ignore')
        if not line:
            continue
        if line.startswith("PROVISION_JSON:"):
            raw = line.removeprefix("PROVISION_JSON:").strip()
            data = json.loads(raw)
            break

    if data is None:
        raise RuntimeError(
            f"No provisioning output received within "
            f"{READ_TIMEOUT_S}s - is the provisioning "
            f"firmware running on {port}?")


    manifest = {
        "device_id": device_id,
        "device_class": device_class,
        "provisioned_at": datetime.now(timezone.utc).isoformat(),
        "atecc608a_serial": data["chip_serial"],
        "public_key_hex": data["public_key"],
        "firmware_hash_hex": data["firmware_hash"],
        "firmware_version": data["firmware_version"],
        "key_slot": data["key_slot"],
        "hash_slot": data["hash_slot"],
        "slots_locked": False,
        "provisioning_tool_version": "0.1.0",
    }

    out_dir = Path("provisioning/devices")
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"{device_id}.json"
    out_path.write_text(json.dumps(manifest, indent=2))

    print(f"Wrote {out_path}")
    print("Register with gateway using:")
    print(f"    python3 register_device.py {out_path}")


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--port", required=True)
    p.add_argument("--device-id", required=True)
    p.add_argument("--device-class", required=True)
    args = p.parse_args()
    capture(args.port, args.device_id, args.device_class)

