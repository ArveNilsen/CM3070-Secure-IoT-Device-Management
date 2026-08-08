from fastapi import APIRouter, HTTPException
from registry.store import DeviceRegistry

router = APIRouter(prefix="/manage")
registry: DeviceRegistry = None     # Dependency injected in main.py

@router.post("/{public_key_id}/quarantine")
async def quarantine_device(public_key_id: str):
    try:
        registry.quarantine(public_key_id)
    except ValueError as e:
        raise HTTPException(status_code=404, detail=str(e))
    return {"status": "quarantined",
            "device": public_key_id}


@router.get("/devices")
async def list_devices():
    devices = registry.all_devices()
    return [{
        "public_key_id": d.public_key_id,
        "device_class": d.device_class,
        "state": d.state,
        "active_caps": d.active_caps,
        "capabilities": d.capabilities,
        "enrolled_at": d.enrolled_at,
    } for d in devices]


@router.get("/{public_key_id}/audit")
async def device_audit(public_key_id: str):
    return registry.audit_log(public_key_id)
