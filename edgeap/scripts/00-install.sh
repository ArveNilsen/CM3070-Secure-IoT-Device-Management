#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/../config/network.env"

echo "[00-install] Installing required packages..."
apt-get update -qq
apt-get install -y --no-install-recommends \
	hostapd \
	dnsmasq \
	nftables \
	bridge-utils \
	iproute2

# hostapd/dnsmasq might be enabled by default.
# Systemd must own them. This is set in the specific scripts.
systemctl disable --now hostapd 2>/dev/null || true
systemctl disable --now dnsmasq 2>/dev/null || true

echo "[00-install] Done."
