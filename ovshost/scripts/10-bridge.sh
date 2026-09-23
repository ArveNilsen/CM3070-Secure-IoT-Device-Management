#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
source "$DIR/config/network.env"

# --may-exist flag for making the script idempotent
ovs-vsctl --may-exist add-br "${BRIDGE_IFACE}"
ovs-vsctl --may-exist add-port "${BRIDGE_IFACE}" "${UPLINK_IFACE}"

if systemctl is-active --quiet NetworkManager; then
		echo "[10-bridge] Excluding OVS interface from MetworkManager..."
		cat > /etc/NetworkManager/conf.d/10-unmanaged-ovs.conf <<EOF
[keyfile]
unmanaged-devices=interface-name:${UPLINK_IFACE};interface-name:${BRIDGE_IFACE}
EOF
		systemctl restart NetworkManager
fi

ip addr add "${BRIDGE_IP}/24" dev "${BRIDGE_IFACE}" 2>/dev/null || true
ip link set "${BRIDGE_IFACE}" up
