"""
Registers a provisioned device's public key and expected firmware hash with the
gateway's trust store. 

Usage:
    python3 register_device devices/esp32-001.json
"""
import json, sys
import sqlite

def register(manifest_path: str):
    with open(manifest_path) as f:
        manifest = json.load(f)

    print(f"Registering device: {manifest['device_id']}")
    print(f"    Class:  {manifest['device_class']}")
    print(f"    Pubkey: {manifest['public_key_hex'][:16]}...")
    confirm = input("Confirm registration? [y/N] ")
    if confirm.lower() != 'y':
        print("Aborted.")
        return

    # Write into gateway's registered devices store.
    # TODO: Consider REST call
    conn = sqlite3.connect("gateway/trust_store.db")
    conn.execute("""
        INSERT INTO register_devices
            (public_key_id, device_class, public_key_hex, firmware_hash_hex, 
             registered_at)
        VALUES (?, ?, ?, ?, datetime('now'))
    """, (manifest['device_id'],
          manifest['device_class'],
          manifest['public_key_hex'],
          manifest['firmware_hash_hex']))
    conn.commit()
    print("Registered.")


if __name__ == "__main__":
    register(sys.argv[1])
                           
