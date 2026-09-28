# Device Provisioning

This directory contains the one-time provisioning workflow that establishes a
hardware-rooted identity for each ESP32 device before deployment. Provisioning
is intentionally separated from the main application firmware and from gateway
operation. It represents the physical/manufacturing trust boundary described
in the report section Design.

## What provisioning does
1. Generates an ECDSA P-256 key pair inside the ATECC608A's secure element.
   The private key is generated on-chip and is never transmitted, logged, or
   readable. Only the public key leaves the device.

2. Computes and stores a SHA-256 hash of the application firmware in a locked
   data slot, approximating a measured-boot property (report section Design).

3. Produces a device manifest file recording the public key and firmware hash,
   for out-of-band registration with the gateway.

## Running provisioning

Prerequisites:
Generate gateway signing key.

Step 1 - flash the provisioning firmware:
    `idf.py -p /dev/ttyUSB0 flash`

Step 2 - capture provisioning output:
```
python3 capture.py --port /dev/ttyUSB0 \
                       --device-id esp32-001 \
                       --device-class sensor
```

Expected output:
```
Listening on /dev/ttyUSB0 for provisioning output...
Wrote provisioning/devices/esp32-001.json
Register with gateway using:
    python3 register_device.py [...]
```

Step 3 -inspect the manifest:
```
cat provisioning/devices/esp32-001.json
```

You should see a JSON file containing the device's public key, firmware hash,
and metadata. This is the complete out-of-band artefact this step produces.

Step 4 - register with gateway:
```
python3 register_device.py devices/esp32-001.json
```

Step 5 - flash the main application firmware:
```
idf.py -p /dev/ttyUSB0 flash -DIDENTITY_STUB_MODE=0
```

## Verifying provisioning succeeded

Run:
```
python3 verify_provisioning.py esp32-001
```

This confirms the device can sign a test challenge and that the gateway's
registered public key correctly verifies it. End-to-end proof the identity
is usable before deployment.

## Scope note (PoC limitation)

Slot locking (making the identity permanent and non-reprovisionable) is
implemented but not invoked by default. See `slots_locked: false`in each
manifest. This is a deliberate scoping decision (see report section Evaluation,
for discussion). The `lock_device.py`script implements this step for
completeness but is not run against the devices used in this thesis's
demonstration.
