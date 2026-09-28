# edge-ap

Deploys the edge access point: hostap (client-isolated Wi-Fi), a bridging
nftables firewall, and DHCP, on a radio-facing host.
The edge-ap communicated with the OVS policy host via ethernet.

## Hardware
Raspberry Pi (tested with Pi 5), a USB Wi-Fi adapter for the AP radio, and Ethernet to ovs-host.
The onboard Wi-Fi is kept in this proof-of-concept for convenient management from dev machine.

## Configure
Edit `config/network.env`

## Deploy
Edit IP address in `deploy.sh`
`./deploy.sh` syncs this to `/opt/edge-ap`
ssh to edge-ap and run `sudo ./setup.sh`
Verify deployment with `bash /opt/edge-ap/scripts/90-verify.ah`

Idempotent - safe to re-run after config changes.
