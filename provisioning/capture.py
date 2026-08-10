"""
Captures provisioning output from ESP32 serial console and writes a device
maifest file. Start this script before executing the provisioning firmware
on the device.

Usage:
    python3 capture.py --port /dev/ttyUSB0 \
                       --device-id esp32-001 \
                       --device-class sensor
"""
import serial, jsonm argparse
from datetime import datetime, timezone

def capture(port: str, device_id: str, device_class: str):
    ser = serial.Serial(port, 115200, timeout=30)
    print(f"Listening on {port} for provisioning output...")

    while True:
        line = ser.readline().decode(errors='ignore')
        if line.startswith("PROVISION_JSON:"):
            raw = line.removeprefix("PROVISION_JSON:").strip()
            data = json.loads(raw)
            break
        else:
            raise RuntimeError("No provisioing output received")

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

    out_path = f"provisioning/devices/{device_id}.json"
    with open(out_path, "w") as f:
        json.dump(manifest, f, indent=2)

    print(f"Wrote {out_path}")
    print(f"Register with gateway using:")
    print(f"    python3 register_device.py {out_path}")


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--port", required=True)
    p.add_argument("--device-id", required=True)
    p.add_argument("--device-class", required=True)
    args = p.parse_args()
    capture(args.port, args.device_id, args.device_class)

