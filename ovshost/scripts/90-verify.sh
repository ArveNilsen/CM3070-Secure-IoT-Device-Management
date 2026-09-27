#!/usr/bin/env bash
set -uo pipefail
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

echo "[90-verify] Checking ovs-host deployment..."
echo ""

echo "-- Services --"
check "openvswitch-switch active" \
		"systemctl is-active --quiet openvswitch-switch"
check "dnsmasq active" \
		"systemctl is-active --quiet dnsmasq"
check "ovs-baseline-flows ran successfully" \
		"systemctl is-active --quiet ovs-baseline-flows"

echo ""
echo "-- Bridge and ports --"
check "br-iot exists (OVSDB)" \
		"ovs-vsctl br-exists ${BRIDGE_IFACE}"
check "br-iot is UP (kernel)" \
		"ip link show ${BRIDGE_IFACE} | grep -q 'state UP\|UNKNOWN'"
check "br-iot has address ${BRIDGE_IP}" \
		"ip addr show ${BRIDGE_IFACE} | grep -q ${BRIDGE_IP}"
check "uplink ${UPLINK_IFACE} is a bridge port (OVSDB)" \
		"ovs-vsctl list-ports  ${BRIDGE_IFACE} | grep -q ${UPLINK_IFACE}"
check "uplink ${UPLINK_IFACE} has carrier" \
		"ip link show ${UPLINK_IFACE} | grep -q LOWER_UP"
check "NetworkManager does not manage bridge interfaces" \
		"! nmcli device status 2>/dev/null | grep -E '${BRIDGE_IFACE}|${UPLINK_IFACE}' | grep -qv unmanaged"

echo ""
echo "-- Flow table --"
check "default-deny rule present" \
		"ovs-ofctl dump-flows ${BRIDGE_IFACE} | grep -q 'priority=0 actions=drop'"
check "exactly 6 baseline flow rules present" \
		"[ \$(ovs-ofctl dump-flows ${BRIDGE_IFACE} | grep -c 'priority=') -eq 6 ]"
check "ARP rule present" \
		"ovs-ofctl dump-flows ${BRIDGE_IFACE} | grep -q 'priority=50,arp'"
check "DHCP rules present (bidirectional)" \
		"[ \$(ovs-ofctl dump-flows ${BRIDGE_IFACE} | grep -c 'priority=40,ip,udp') -eq 2 ]"
check "enrollment-bootstrap rules present (bidirectional)" \
	"[ \$(ovs-ofctl dump-flows ${BRIDGE_IFACE} | grep -c 'priority=100') -eq 2 ]"

echo ""
if [ "$fail" -eq 0 ]; then
		echo "All check passed."
else
		echo "One or more checks failed, see output above."
		exit 1
fi
