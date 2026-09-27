#!/usr/bin/env python3
"""
Captures evidence for NoCapabilityWithoutEnrollment and QuarantineIsIsolated.
Produces a JSON object.

Based on the manual tests:
# before quarantine
sudo ovs-ofctl dump-flows br-iot | grep priority=300  # (none yet)
# quarantine, then have device attempt telemetry
sudo ovs-ofctl dump-flows br-iot | grep priority=300  # n_packets > 0
"""
import json, subprocess, time
from registry.store import DeviceRegistry
from pep.decision import should_accept
from policy.manifests import Capability

TEST_MAC = "38:18:2B:8B:F9:48"
BRIDGE = "br-iot"


def ovs_rule_present(mac: str) -> bool:
    out = subprocess.run(
        ["sudo", "-n", "ovs-ofctl", "dump-flows", BRIDGE],
        capture_output=True, text=True, check=True).stdout
    return f"dl_src{mac}" in out and "priority=200" in out


def ovs_drop_counter(mac: str) -> int:
    result = subprocess.run(
        ["sudo", "-n", "ovs-ofctl", "dump-flows", BRIDGE],
        capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"ovs-ofctl dump-flows failed: {result.stderr.strip()}")
    for line in result.stdout.splitlines():
        if f"dl_src={mac}" not in line:
            continue
        if f"priority={priority}" not in line:
            continue
        match = re.search(r"n_packets=(\d+)", line)
        if match is None:
            raise RuntimeError(
                f"Matched rule but could not parse n_packets: {line!r}")
        return int(match.group(1))
    
    return 0


def evidence_no_capability_without_enrollment(registry):
    device = registry.get("esp32-001") # unenrolled
    app_layer = should_accept(
        device_state="unenrolled" if device is None else device.state,
        active_caps=0,
        required_caps=int(Capability.PUBLISH_TELEMETRY))
    net_layer = ovs_rule_present(TEST_MAC)
    return {
        "property": "NoCapabilityWithoutEnrollment",
        "app_layer_pep_accepts": app_layer, # false
        "network_layer_rule_present": net_layer, # false
    }


def evidence_quarantine_is_isolated(registry):
    before = {
        "app_layer_pep_accepts": should_accept(
            "enrolled", registry.get("esp32-001").active_caps,
            int(Capability.PUBLISH_TELEMETRY)),
        "network_layer_rule_present": ovs_rule_present(TEST_MAC),
        "drop_counter": ovs_drop_counter(TEST_MAC), # 0 is good!
    }
    registry.quarantine("esp32-001")
    time.sleep(1) # allow for processing

    after_quarantine = {
        "after_layer_pep_accepts": should_accept(
            "quarantined", 0, int(Capability.PUBLISH_TELEMETRY)),
        "network_layer_rule_present": ovs_rule_present(TEST_MAC),
        "drop_counter": ovs_drop_counter(TEST_MAC), # 0 is good!
    }

    print("Trigger a telemetry publish attempt from the quarantined device, "
          "then press Enter...")
    input()

    after_traffic = {
        "drop_counter": ovs_drop_counter(TEST_MAC), # > 0
    }

    return {
        "property": "QuarantineIsIsolated",
        "before": before,
        "after_quarantine": after_quarantine,
        "after_traffic_attempt": after_traffic,
    }


if __name__ == "__main__":
    registry = DeviceRegistry("registry.db")
    results = [
        evidence_no_capability_without_enrollment(registry),
        evidence_quarantine_is_isolated(registry),
    ]
    print(json.dumps(results, indent=2))
    
