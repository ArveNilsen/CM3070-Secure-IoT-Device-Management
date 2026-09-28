# Device firmware

ESP-IDF v5.3.6, ESP32. Two apps: `apps/device-main` (application) and
`apps/provisioning_tool` (one-time identity setup).

## Build order matters
`provisioning_tool` embeds the SHA-256 of the built `device-main` binary
(`tools/compute_firmware_hash.py` generates `firmware_hash.hpp`).
Build `device-main` first, hash it, then build `provisioning_tool`. Do not
rebuild `device-main` afterwards, or its hash will no longer match.

## Hardware
ATECC608A on I2C: SDA GPIO21, SCL GPIO22, address 0xC0, external pull-ups.

## Host tests
`tests/host/` builds the ESP-IDF-independent components (Wi-Fi state machine)
for desktop.

## Components
`identity` (secure element), `enrollment`, `configs` (NVS), `wifi_station`,
capability enforcer.

## Other apps
I2C scanner, build system testing, ATECC608 playground.
