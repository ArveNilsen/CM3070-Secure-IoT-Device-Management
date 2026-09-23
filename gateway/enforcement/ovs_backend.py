import subprocess
from .backend import NetworkEnforcementBackend

class OVSBackend(NetworkEnforcementBackend):
    def __init__(self, bridge="br_iot", gateway_ip="10.42.0.1", 
                 mqtt_port=1883):
        self.bridge = bridge
        self.gateway_ip = gateway_ip
        self.mqtt_port = mqtt_port


    def _ofctl(self, *args):
        """
        Currently refactored out.
        Consider removing if no need.
        """
        subprocess.run(
            ["ovs-ofctl", *args[:-1], self.bridge, args[-1]]
            if args[-1].startswith("priority")
            else ["ovs-ofctl", *args, self.bridge], 
            check=True)


    def quarantine(self, mac_adress: str) -> None:
        # Remove all per-device flow, then install drop.
        # A quarantined device must not fall through to the baseline.
        subprocess.run(
            ["ovs-ofctl", "del-flows", self.bridge,
             f"dl_src={mac_adress}"], check=True)
        subprocess.run(
            ["ovs-ofctl", "del-flows", self.bridge,
             f"priority=300,dl_src={mac_adress},actions=drop"],
            check=True)


    def restore(self, mac_adress: str) -> None:
        """
        Caller is responsible for a follow-up apply_capabilities().
        """
        subprocess.run(
            ["ovs-ofctl", "del-flows", self.bridge,
             f"dl_src={mac_adress}"], check=True)
        

    def apply_capabilities(self, mac_adress: str, active_caps: int) -> None:
        """
        active_caps == 0: no allow rule installed.
        """
        subprocess.run(
            ["ovs-ofctl", "del-flows", self.bridge,
             f"dl_src={mac_adress},priority=200"], check=True)
        if active_caps != 0:
            subprocess.run([
                "ovs-ofctl", "add-flow", self.bridge,
                 f"priority=200,dl_src={mac_adress},ip,"
                 f"nw_dst={self.gateway_ip},tcp,"
                 f"tp_dst={self.mqtt_port},actions=normal"
            ], check=True)
            subprocess.run([
                "ovs-ofctl", "add-flow", self.bridge,
                 f"priority=200,dl_src={mac_adress},ip,"
                 f"nw_src={self.gateway_ip},tcp,"
                 f"tp_src={self.mqtt_port},actions=normal"
            ], check=True)


