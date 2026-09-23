#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
source "$DIR/config/network.env"

echo "[30-firewall] Rendering nftables.conf..."

if ! lsmod | grep -q br_netfilter; then
	echo "[30-firewall] Loading br_netfilter module..."
	modprobe br_netfilter
	echo "br_netfilter" > /etc/modules-load.d/br_netfilter.conf
fi

sed \
	-e "s/@WIFI_IFACE@/${WIFI_IFACE}/g" \
	-e "s/@UPLINK_IFACE@/${UPLINK_IFACE}/g" \
	"$DIR/config/nftables.conf" > /etc/nftables.conf

systemctl enable nftables
systemctl restart nftables

echo "[30-firewall] Done."
nft list ruleset
