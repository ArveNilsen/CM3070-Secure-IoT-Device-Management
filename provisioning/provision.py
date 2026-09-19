#!/usr/bin/env python3
"""
Device provisioning tool.

Generates an ATECC608 keypar on-chip (via the provisioning firmware,
ran separately). See PROVISIONING.md

Captures the output, then generates a pre-populated NVS partition image
with identity and network configuration.

Usage:
    python provision.py \
        --device-id esp32-001 \
        --device-class sensor \
        --ssid MyNetwork --password secret123 \
        --gateway-host 192.168.86.50 \
        --pubkey-capture provisioning/devices/esp32-001.json \
        --port /dev/ttyUSB0
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

# NVS partition layout
NVS_CSV_TEMPLATE = """key,type,encoding.value
device_cfg,namespace,,
wifi_ssid,data,string,{wifi_ssid}
wifi_pass,data,string,{wifi_password}
gw_host,data,string,{gateway_host}
device_id,data,string,{device_id}
pubkey_id,data,string,{public_key_id}
enrolled,data,u8,0
"""


def generate_nvs_csv(args, public_key_id: str, out_path: Path):
    content = NVS_CSV_TEMPLATE.format(
        wifi_ssid=args.ssid,
        wifi_password=args.password,
        gateway_host=args.gateway_host,
        device_id=args.device_id,
        public_key=public_key_id,
    )

    out_path.write_text(content)
    print(f"Wrote NVS CSV: {out_path}")


def generate_nvs_binary(csv_path: Path, bin_path: Path,
                        partition_size: str = "0x6000"):
    """
    Invokes ESP-IDF's nvs_partition_gen.py to produce a
    flashable binary image from the CSV definition above.
    """
    idf_path = subprocess.check_output(
        ["python", "-c", "import os; print(os.environ['IDF_PATH'])"]
    ).decode().strip()

    gen_script = (Path(idf_path) / "components" / "nvs_flash"
                  / "nvs_partition_generator" / "nvs_partition_gen.py")

    subprocess.run([
        sys.executable, str(gen_script), "generate",
        str(csv_path), str(bin_path), partition_size
    ], check=True)
    print(f"Generated NVS partition image: {bin_path}")


def flash_nvs_partition(bin_path: Path, port: str,
                        offset: str = "0x9000"):
    """
    Flashes the generated NVS image to the device_cfg
    partition's offset.

    Offset must match partitions.csv, a mismatch will
    corrupt other partitions.
    """
    subprocess.run([
        "esptool.py", "--port", port, "write_flash",
        offset, str(bin_path)
    ], check=True)
    print(f"Flashed NVS partition to {port} at {offset}")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--device-id", required=True)
    p.add_argument("--device-class", required=True,
                   choices=["sensor", "actuator", "controller"])
    p.add_argument("--ssid", required=True)
    p.add_argument("--password", required=True)
    p.add_argument("--gateway-host", required=True)
    p.add_argument("--pubkey-capture", required=True,
                   help="Path to the JSON manifest")
    p.add_argument("--port", required=True)
    p.add_argument("--out-dir", default="provisioning/nvs_images")
    args = p.parse_args()

    capture_path = Path(args.pubkey_capture)
    if not capture_path.exists():
        print(f"ERROR: {capture_path} not found -- run "
              f"capture.py (ATECC608 key generation) first")
        sys.exit(1)

    capture_data = json.loads(capture_path.read_text())
    public_key_id = capture_data["device_id"]

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    csv_path = out_dir / f"{args.device_id}.csv"
    bin_path = out_dir / f"{args.device_id}.bin"

    generate_nvs_csv(args, public_key_id, csv_path)
    generate_nvs_binary(csv_path, bin_path)

    confirm = input(f"Flash NVS image to {args.port}? "
                    f"This writes identity/network config "
                    f"for '{args.device_id}'. [y/N] ")
    if confirm.lower() != 'y':
        print("Aborted - image generated but not flashed.")
        return

    flash_nvs_partition(bin_path, args.port)
    print(f"Provisioning complete for {args.device_id}.")



if __name__ == "__main__":
    main()
