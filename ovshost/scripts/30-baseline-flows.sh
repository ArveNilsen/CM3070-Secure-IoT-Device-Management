#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd  "$(dirname "$0")/.." && pwd)"
source "$DIR/config/network.env"

echo "[30-baseline-flows] Resetting flow table..."

# clears everything
ovs-ofctl del-flows "${BRIDGE_IFACE}"

# baseline Zero Trust -> drop all packets
ovs-ofctl add-flow "${BRIDGE_IFACE}" \
		"priority=0,actions=drop"

# ARP must work for IP-layer communication
ovs-ofctl add-flow "${BRIDGE_IFACE}" \
		"priority=50,dl_type=0x0806,actions=normal"

# DHCP must work
ovs-ofctl add-flow "${BRIDGE_IFACE}" \
		"priority=40,udp,tp_src=68,tp_dst=67,actions=normal"
ovs-ofctl add-flow "${BRIDGE_IFACE}" \
		"priority=40,udp,tp_src=67,tp_dst=68,actions=normal"

ovs-ofctl add-flow "${BRIDGE_IFACE}" \
		"priority=100,ip,nw_dst=${BRIDGE_IP},tcp,tp_dst=${GATEWAY_PORT_ENROLL},actions=normal"

echo "[30-baseline-flows] Done."
ovs-ofctl dump-flows "${BRIDGE_IFACE}"
