import json
import paho.mqtt.client as mqtt

import sys
from pathlib import Path

_PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(_PROJECT_ROOT))

from registry.store import DeviceRegistry
from policy.manifests import Capability
from enforcement.backend import NullBackend
from pep.decision import should_accept

registry = DeviceRegistry("registry.db")
backend = NullBackend() # Dependency injected at main

TOPIC_TELEMETRY = "device/+/telemetry"

def on_message(client, userdata, msg):
    parts = msg.topic.split("/")
    public_key_id = parts[1]

    device = registry.get(public_key_id)
    if device is None:
        print(f"REJECT: unknown device {public_key_id}")
        return

    if not should_accept(device.state, device.active_caps, 
                         int(Capability.PUBLISH_TELEMETRY)):
        print(f"REJECT: {public_key_id} (state={device.state})")
        registry._audit(public_key_id, "rejected", f"topic{msg.topic}")
        return

    # Accepted - process normally
    payload = json.loads(msg.payload)
    print(f"ACCEPT: {public_key_id} -> {payload}")
    registry._audit(public_key_id, "telemetry_accepted", json.dumps(payload))


def run():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_message = on_message
    client.connect("localhost", 1883)
    client.subscribe(TOPIC_TELEMETRY)
    client.loop_forever()


if __name__ == "__main__":
    run()
