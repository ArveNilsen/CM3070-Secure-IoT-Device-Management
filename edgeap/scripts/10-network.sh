#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
source "$DIR/config/network.env"
echo "[10-network] Writing systemd-networkd config..."

mkdir -p /etc/systemd/network

cat > /etc/systemd/network/10-${BRIDGE_IFACE}.netdev <<EOF
[NetDev]
Name=${BRIDGE_IFACE}
Kind=bridge
EOF

cat > /etc/systemd/network/20-${BRIDGE_IFACE}.network <<EOF
[Match]
Name=${BRIDGE_IFACE}

[Network]
Address=${BRIDGE_IP}/24
ConfigureWithoutCarrier=yes
EOF

cat > /etc/systemd/network/30-${UPLINK_IFACE}.network <<EOF
[Match]
Name=${UPLINK_IFACE}

[Network]
Bridge=${BRIDGE_IFACE}
EOF

# hostapd adds its own bridge
# VERIFY THIS

systemctl enable systemd-networkd
systemctl restart systemd-networkd

echo "[10-network] Done. Bridge ${BRIDGE_IFACE} should be up with ${BRIDGE_IP}."
