### Removing a device from the registry

Use this when a device's local state has been reset but the gateway still has
an enrollment record for it. Convenient recovery from "state mismatch" 
diagnostic.

- Confirm the device is actually in the registry:
```bash
sqlite3 gateway/registry.db \
    "SELECT public_key_id, state, enrolled_at FROM devices WHERE public_key_id='<device-id>';"
```

- Script for removing the device (the recommended approach)
```bash
python3 gateway/remove_device.py <device-id>
```

- Or manually via SQL
```bash
sqlite3 gateway/registry.db \
    "DELETE FROM devices WHERE public_key_id='<device-id>';"
```

- Re-run the enrollment from the device.
- If the device should be fully re-provisioned, with a new hardware identity,
also remove it from the trust store and re-capture per the provisioning guide.
