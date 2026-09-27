#!/usr/bin/env bash
set -euo pipefail

OVS_HOST="${1:-arve@192.168.86.77}"

echo "Syncing gateway to $OVS_HOST..."

rsync -avz --delete \
    --exclude={'.git','__pycache__','.venv','/test/','.pytest_cache','.hypothesis','.ruff_cache','registry.db','trust_store.db','gateway_private_key.pem'} \
    ./ "$OVS_HOST:/opt/gateway/"
