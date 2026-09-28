#!/usr/bin/env bash
set -euo pipefail

OVS_HOST="${1:-arve@192.168.86.77}"

echo "Syncing dashboard to $OVS_HOST..."

rsync -avz --delete --exclude '.git' ./ "$OVS_HOST:/opt/dashboard/"
