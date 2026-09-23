#!/usr/bin/env bash
set -euo pipefail

PI_HOST="${1:-arvenilsen@192.168.86.136}"

echo "Syncing edge-ap to $PI_HOST..."

rsync -avz --delete --exclude '.git' ./ "$PI_HOST:/opt/edge-ap/"

echo "Running setup.sh remotely..."
ssh "$PI_HOST" 'cd /opt/edge-ap && sudo ./setup.sh'
