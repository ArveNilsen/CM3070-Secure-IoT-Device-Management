#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd  "$(dirname "$0")/.." && pwd)"
source "$DIR/config/network.env"

echo "[20-dhcp] Writing dnsmasq config..."

cat > /etc/dnsmasq.d/edge-iot.conf <<EOF
interface=${BRIDGE_IFACE}
bind-interfaces
dhcp-range=${DHCP_RANGE_START},${DHCP_RANGE_END},${DHCP_LEASE_TIME}
dhcp-authoritative
EOF

systemctl enable dnsmasq
systemctl restart dnsmasq

echo "[20-dhcp] Done."
