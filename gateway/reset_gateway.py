#!/usr/bin/env python3
"""
Fully resets the gateway's persistent state: registry.db and trust_store.db.

The gateway's own signing key (gateway_private_key.pem) is preserved by default.
Use --include-gateway-key to also remove it. Only appropriate if re-provisioning
every device, including re-embedding a new gateway key.

Usage:
    python3 reset_gateway.py [--include-gateway-key] [--yes]
"""
import argparse
import sys
from pathlib import Path

GATEWAY_DIR = Path(__file__).parent

FILES_TO_RESET = [
    GATEWAY_DIR / "registry.db",
    GATEWAY_DIR / "trust_store.db",
]

GATEWAY_KEY_FILE = GATEWAY_DIR / "gateway_private_key.pem"


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--include-gateway-key", action="store_true",
                   help="Also remove the gateway's own signing key. Only do "
                        "this  if re-provisioning every device.")
    p.add_argument("--yes", action="store_true", help="Skip confirmation prompt.")
    args = p.parse_args()

    targets = list(FILES_TO_RESET)
    if args.include_gateway_key:
        targets.append(GATEWAY_KEY_FILE)

    existing = [f for f in targets if f.exists()]
    if not existing:
        print("Nothing to reset, no state files found.")
        return

    print("The following files will be permanently deleted:")
    for f in existing:
        print(f"    {f}")
    if args.include_gateway_key:
        print("\n   WARNING: this includes the gateway's signing key. "
              "Devices must be re-provisioned.")

    if not args.yes:
        confirm = input("\nProceed? This cannot be undone. [y/N] ")
        if confirm.lower () != 'y':
            print("Aborted.")
            return

    for f in existing:
        f.unlink()
        print(f"Removed {f}")

    print("\nGateway state reset. Databases are recreated on startup.")


if __name__ == "__main__":
    main()
