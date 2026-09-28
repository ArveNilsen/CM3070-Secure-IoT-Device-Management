# Provisioning

Establishes a device's hardware identity and registers it with the gateway
out-of-band, before the device ever touches the network.

## Workflow (per device)
1. Flash `devices/apps/provisioning_tool`. It generates the keypair on the
   ATECC608A and prints a `PROVISION_JSON:` line.
2. `capture.py` writes `devices/<id>.json`.
3. `register_device.py` adds the public key to the gateway trust store; sync
   `trust_store.db` to the gateway host.
4. `provision.py` writes Wi-Fi/gateway/identity config to the device's NVS.

## Warning: irreversible
The provisioning firmware locks the ATECC608A config and data zones. This
cannot be undone on that chip.


