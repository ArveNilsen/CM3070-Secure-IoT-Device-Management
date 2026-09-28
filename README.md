# CM3070 Final Project
## 8.1 Secure IoT Device Management in a Safety-critical Smart Environment

| Directory | Contents |
|---|---|
| `specs/` | Alloy and TLA+ specifications (Design chapter) |
| `gateway/` | FastAPI gateway, registry, PEP, OVS enforcement backend, pytest suite |
| `devices/` | ESP32 firmware (ESP-IDF) and host-side tests |
| `provisioning/` | Out-of-band identity capture and registration tooling |
| `edge-ap/`, `ovs-host/` | Deployment scripts for the two network hosts |
| `dashboard/` | Single-file operator dashboard |

## Where to start
1. `specs/` for the formal model, then `gateway/` tests, which map to the
   traceability table in the report (Evaluation).
2. The demo video shows the full system running on hardware.

## Runs without hardware
Gateway test suite (`gateway/README.md`); TLA+/Alloy specs (TLC / Alloy Analyzer).

## Requires hardware
End-to-end enrollment needs an ESP32 with an ATECC608A, plus the two-host
network setup. This is what the video demonstrates.

## Secrets
No keys or credentials are committed. The gateway signing key and Wi-Fi
credentials are generated or supplied locally (see `gateway/`, `edge-ap/`).
