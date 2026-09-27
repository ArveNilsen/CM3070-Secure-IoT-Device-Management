#!/usr/bin/env bash
# provisioning/sync_trust_store.sh
set -euo pipefail
OVS_HOST="${1:-arve@192.168.86.77}"
rsync -avz ../gateway/trust_store.db "$OVS_HOST:/opt/gateway/trust_store.db"
echo "trust_store.db synced to $OVS_HOST."
