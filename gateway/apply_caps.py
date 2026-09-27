#!/usr/bin/env python3
"""
Manually apply capabilities, for testing.

Usage:
    python3 apply_caps.py esp-32-001 [--caps] [0x00] (optional overrride mask)
"""
import sys
import argparse

sys.path.insert(0, "/opt/gateway/")
from registry.store import DeviceRegistry
from enforcement.ovs_backend import OVSBackend


def main():
    p = argparse.ArgumentParser()
    p.add_argument("public_key_id")
    p.add_argument("--caps", type=lambda x: int(x, 0),
                   default=None,
                   help="Overrride capability mask")
    args = p.parse_args()

    registry = DeviceRegistry("registry.db")
    device = registry.get(args.public_key_id)
    if device is None:
        print(f"'{args.public_key_id}' has no recorded MAC.")
        return

    caps = args.caps if args.caps is not None else device.active_caps
    print(f"Applying capabilities for {args.public_key_id} "
          f"(MAC={device.mac_address}): mask=0x{caps:08x}")

    backend = OVSBackend()
    backend.apply_capabilities(device.mac_address, caps)

    registry.close()
    print("Done.")


if __name__ == "__main__":
    main()
