# edge-ap

Deploys the edge access point: hostap (client-isolated Wi-Fi), a bridging
nftables firewall, and DHCP, on a radio-facing host.
The edge-ap communicated with the OVS policy host via ethernet.

## Deploy
sudo ./setup.sh

Idempotent - safe to re-run after config changes.
