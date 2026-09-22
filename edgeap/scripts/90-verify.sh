#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
source "$DIR/config/network.env"

fail=0
check() {
		if eval "$2"; then
				echo " [OK]		$1"
		else
				echo " [FAIL]	$1"
				fail=1
		fi
}

echo "[90-verify] Checking edge-ap deployment..."

check "systemd-networkd active" \
		"systemctl is-active --quiet systemd-networkd"
check "bridge ${BRIDGE_IFACE} exists" \
		"ip link show ${BRIDGE_IFACE} &>/dev/null"
check "bridge has address ${BRIDGE_IP}" \
		"ip addr show ${BRIDGE_IFACE} | grep -q ${BRIDGE_IP}"
check "hostapd-edge active" \
		"systemctl is-active --quiet hostapd-edge"
check "dnsmasq active" \
		"systemctl is-active --quiet dnsmasq"
check "nftables active" \
		"systemctl is-active --quiet nftables"
check "br_netfilter loaded" \
		"lsmod | grep -q br_netfilter"
check "bridge nftables ruleset present" \
		"nft list ruleset | grep -q 'table bridge filter'"

echo ""
if [ "$fail" -eq 0 ]; then
		echo "All check passed."
else
		echo "One or more checks failed, see output above."
		exit 1
fi
