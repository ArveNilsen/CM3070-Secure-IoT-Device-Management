#!/usr/bin/env bash
set -euo pipefail

OVS_HOST="${1:-arve@192.168.86.77}"

echo "Syncing edge-ap to $OVS_HOST..."

rsync -avz --delete --exclude '.git' ./ "$OVS_HOST:/opt/ovs-host/"

echo "Running setup.sh remotely..."
ssh -t "$OVS_HOST" 'cd /opt/ovs-host && sudo ./setup.sh'
