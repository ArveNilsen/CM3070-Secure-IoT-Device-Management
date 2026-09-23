#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
source "$DIR/config/network.env"

# --may-exist flag for making the script idempotent
ovs-vsctl --may-exist add-br "${BRIDGE_IFACE}"
ovs-vsctl --may-exist add-port "${BRIDGE_IFACE}" "${UPLINK_IFACE}"

ip addr add "${BRIDGE_IP}/24" dev "${BRIDGE_IFACE}" 2>/dev/null || true
ip link set "${BRIDGE_IFACE}" up
