### Add the following to systemd

NOTE: Path to script must be fixed upon deployment
```bash
# /etc/sytemd/system/ovs-baseline-flows.service
[Unit]
Description=Re-apply baseline OVS flow rules
After=openvswitch-switch.service
Requires=openvswitch-switch.service

[Service]
Type=oneshot
ExecStart=/path/to/ovs-host/scripts/30-baseline-flows.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```
