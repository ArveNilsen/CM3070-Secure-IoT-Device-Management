# Gateway

FastAPI service handling enrollment, a SQLite device registry, an MQTT policy enforcement point (PEP), and a network enforcement backend.

# Run
```
uv sync
python3 generate_gateway_key.py
GATEWAY_STUB_MODE=0 uv run main.py
```

Configuration: `firmware_hashes.json` 
Approved firmware SHA-256 per device class, startup fails if malformed or empty.

## Test
```
uv run pytest
```

Tests map to formal properties. Needs no hardware.

## Layout
`enrollment/` protocol and attestation, `registry/` state and trust store,
`policy/` capability ceilings, `pep/` MQTT enforcement, `enforcement/`
`NetworkEnforcementBackend` and its OVS implementation, `management/` API.

## Note
`OVSBackend` shells out to `sudo -n ovs-ofctl` and only works on the OVS host.
Elsewhere, enrollment completes at the application layer and OVS failures are
logged as warnings.
