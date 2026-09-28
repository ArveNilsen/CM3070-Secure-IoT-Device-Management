# ove-host

Open vSwitch bridge (`br-iot`) with a default-deny OpenFlow baseline and dnsmasq DHCP.
This script installs the baseline, the rest is managed by the backend.

## Hardware
A linux host (Debian/Ubuntu, apt) with a wired network to edge-ap.

## Configure
Edit `config/network.env`

## Deploy
Edit IP address in `deploy.sh`
`./deploy.sh` syncs this to `/opt/ovs-host`
ssh to ovs-host and run `sudo ./setup.sh`
Verify deployment with `bash /opt/ovs-host/scripts/90-verify.ah`

Idempotent - safe to re-run after config changes.
