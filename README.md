# CM3070 Final Project
## 8.1 Secure IoT Device Management in a Safety-critical Smart Environment

### Project Structure
Root - You are here. Project level information and instructions.
devices - Specific devices in sub folders
gateway - This zero trust components
dashboard - The user interface

### High-level workflow
```
--------------------------------------------------------------------------------
| 1. Flash provisioning firmware to ESP32 (separate app) 
--------------------------------------------------------------------------------
    |
    v
--------------------------------------------------------------------------------
|2. Run provisioning tool over serial
|   -> generates key pair on-chip
|   -> writes firmware hash to data slot
|   -> prints structures JSON to serial console
--------------------------------------------------------------------------------
    |
    v
--------------------------------------------------------------------------------
|3. Host-side script captures serial JSON
|   -> writes to devices/esp32-001.json
|   -> appends to gateway's registered_devices
--------------------------------------------------------------------------------
    |
    v
--------------------------------------------------------------------------------
|4. Lock slots - requires manual confirmation
|   (Optional, explicit, separate step)
|       -> This step is irreversible
--------------------------------------------------------------------------------
    |
    v
--------------------------------------------------------------------------------
|5. Flash main application firmware
|   -> device is now deployable
--------------------------------------------------------------------------------
```

### Device manifest
Stored under `provisioning/devices/*.json`.
A separate field with a boolean value for slots_locked is kept for the proof-of-concept implementation.

Example:
```json
{
  "device_id": "esp32-001",
  "device_class": "sensor",
  "provisioned_at": "2026-08-10T09:32:00Z",
  "atecc608a_serial": "0123ABCD4567EF89",
  "public_key_hex": "AABBCC...64 bytes...",
  "firmware_hash_hex": "1122DD...32 bytes...",
  "firmware_version": "v0.3.0-poc",
  "key_slot": 0,
  "hash_slot": 8,
  "slots_locked": false,
  "provisioning_tool_version": "0.1.0"  
}
```

### Provisioning workflow
[See provisioning user guide](provisioning/PROVISIONING.md)
