import json
import paho.mqtt.client as mqtt

from registry.store import DeviceRegistry
from policy.maifests import Capability

registry = DeviceRegistry("registry.db")

TOPIC_TELEMETRY = "device/+/telemetry"

def on_message(client, userdata, msg):
    parts = msg.topic.split("/")
    public_key_id = parts[1]

    device = registry.get(public_key_id)
    if device is None:
        print(f"REJECT: unknown device {public_key_id}")
        return

    if device.state == "quarantined":
        print(f"REJECT: {public_key_id} is quarantined - dropping message")
        registry._audit(public_key_id, "rejected_quarantined", 
                        f"topic={msg.topic}")
        return

    required_cap = Capability.PUBLISH_TELEMETRY
    if not (device.active_caps & required_cap):
        print(f"REJECT: {public_key_id} lacks PUBLISH_TELEMETRY capability")
        registry._audit(public_key_id, "policy_violation",
                        f"topic={msg.topic} missing_cap=PUBLISH_TELEMETRY")
        return

    # Accepted - process normally
    payload = json.loads(msg.payload)
    print(f"ACCEPT: {public_key_id} -> {payload}")
    registry._audit(public_key_id, "telemetry_accepted", json.dumps(payload))


def run():
    client = mqtt.Client()
    client.on_message = on_message
    client.connect("localhost", 1883)
    client.subscribe(TOPIC_TELEMETRY)
    client.loop_forever()


if __name__ == "__main__":
    run()
