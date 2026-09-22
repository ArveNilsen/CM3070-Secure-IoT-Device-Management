#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
source "$DIR/config/network.env"

echo "[40-hostapd] Rendering hostapd.conf..."

sed \
	-e "s/@WIFI_IFACE@/${WIFI_IFACE}/g" \
	-e "s/@BRIDGE_IFACE@/${BRIDGE_IFACE}/g" \
	-e "s/@SSID@/${SSID}/g" \
	-e "s/@WPA_PASSPHRASE@/${WPA_PASSPHRASE}/g" \
	"$DIR/config/hostapd.conf" > /etc/hostapd/hostapd.conf

cat > /etc/systemd/system/hostapd-edge.service <<EOF
[Unit]
Description=hostapd (edge AP)
After=systemd-networkd.service
Requires=systemd-networkd.service
Wants=network-online.target

[Service]
Type=simple
ExecStart=/usr/sbin/hostapd /etc/hostapd/hostapd.conf
Restart=on-failure
RestartSec=2

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable hostapd-edge
systemctl restart hostapd-edge

echo "[40-hostapd] Done."
