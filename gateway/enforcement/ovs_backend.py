import subprocess
from .backend import NetworkEnforcementBackend

class OVSBackend(NetworkEnforcementBackend):
    def __init__(self, bridge="br-iot", gateway_ip="10.42.0.1", 
                 mqtt_port=1883):
        self.bridge = bridge
        self.gateway_ip = gateway_ip
        self.mqtt_port = mqtt_port


    def _ovs_ofctl(self, *args):
        """
        Helper function
        """
        subprocess.run(["sudo", "-n", "ovs-ofctl", *args], check=True)


    def quarantine(self, mac_address: str) -> None:
        # Remove all per-device flow, then install drop.
        # A quarantined device must not fall through to the baseline.
        self._ovs_ofctl("del-flows", self.bridge, f"dl_src={mac_address}")
        self._ovs_ofctl("del-flows", self.bridge, f"dl_dst={mac_address}")
        self._ovs_ofctl("add-flow", self.bridge,
            f"priority=300,dl_src={mac_address},actions=drop")

    def restore(self, mac_address: str) -> None:
        """
        Caller is responsible for a follow-up apply_capabilities().
        """
        self._ovs_ofctl("del-flows", self.bridge, f"dl_src={mac_address}")
        

    def apply_capabilities(self, mac_address: str, active_caps: int) -> None:
        """
        active_caps == 0: no allow rule installed.
        """
        self._ovs_ofctl("del-flows", self.bridge,
            f"dl_src={mac_address}")
        self._ovs_ofctl("del-flows", self.bridge,
            f"dl_dst={mac_address}")
        if active_caps != 0:
            self._ovs_ofctl("add-flow", self.bridge,
                f"priority=200,dl_src={mac_address},ip,"
                f"nw_dst={self.gateway_ip},tcp,tp_dst={self.mqtt_port},actions=normal")
            self._ovs_ofctl("add-flow", self.bridge,
                f"priority=200,dl_dst={mac_address},ip,"
                f"nw_src={self.gateway_ip},tcp,tp_src={self.mqtt_port},actions=normal")

