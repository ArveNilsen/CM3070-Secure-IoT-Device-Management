#!/usr/bin/env bash

# Builds the application firmware, hashes it, and builds the provisioning
# firmware against the hash.
#
# Run before every provisioning session.
# Do NOT rebuild the application firmware without re-running this.

set -euo pipefail

cd "$(dirname "$0")/.."

echo "== Building application firmware =="
(cd apps/device-main && idf.py build)

echo "== Computing firmware hash =="
python3 tools/compute_firmware_hash.py \
    --bin apps/device-main/build/device-main.bin \
    --out apps/provisioning_tool/include/firmware_hash.hpp

echo "== Building provisioning firmware =="
(cd apps/provisioning-tool && idf.py build)

echo "== Ready =="
echo "1. Flash provisioning firmware: cd apps/provisioning-tool && idf.py -p <PORT> flash monitor"
echo "2. Run capture.py against it"
echo "3. Flash application firmware (do not rebuild): cd apps/device-main && idf.py -p <PORT> flash"
