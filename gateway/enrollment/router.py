import json

#import hashlib, hmac
import time

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel

from enrollment.attestation import sign_manifest, verify_attestation
from policy.manifests import ceiling_for_class
from registry.store import DeviceRegistry

from .nonce_store import NonceStore

registry: DeviceRegistry = None

router      = APIRouter(prefix="/enroll")
nonce_store = NonceStore()

# --- Request / Response models ---

class NonceRequest(BaseModel):
    public_key_id: str
    device_class: str


class NonceResponse(BaseModel):
    nonce: str # hex-encoded
    timestamp: int


class AttestationRequest(BaseModel):
    public_key_id: str
    nonce: str # hex-encoded
    timestamp: int
    firmware_hash: str
    device_class: str
    secure_boot: bool
    signature: str # hex-encoded


class ManifestResponse(BaseModel):
    capabilities: int # bitmask
    manifest_version: int
    gateway_signature: str # hex-encoded

# --- Endpoints ---

@router.post("/nonce", response_model=NonceResponse)
async def request_nonce(req: NonceRequest):
    """
    Phase 1: device announces itself and receives a fresh nonce to include in
    its attestation evidence.
    """
    try:
        ceiling_for_class(req.device_class)
    except ValueError as exc:
        raise HTTPException(
            status_code=400,
            detail=f"Unknown device class: {req.device_class}"
        ) from exc

    nonce = nonce_store.issue(req.public_key_id)

    return NonceResponse(nonce=nonce.hex(), timestamp=int(time.time()))


@router.post("/attest", response_model=ManifestResponse)
async def submit_attestation(req: AttestationRequest):
    """
    Phase 2: device submits signed attestation evidence. Gateway verifies and
    issues manifest.
    """

    # 1. Verify nonce is valid and consume it
    try:
        presented = bytes.fromhex(req.nonce)
    except ValueError as exc:
        raise HTTPException(
            status_code=400, detail="Malformed nonce"
        ) from exc

    if not nonce_store.consume(req.public_key_id, presented):
        raise HTTPException(status_code=401, detail="Invalid or expired nonce")

    # 2. Verify timestamp freshness
    age = abs(time.time() - req.timestamp / 1000)
    if age > 90: # TODO: Remove hardcoded value
        raise HTTPException(status_code=401, detail="Timestamp too stale")

    # 3. Verify device not already enrolled
    if registry.is_enrolled(req.public_key_id):
        raise HTTPException(status_code=409, detail="Device already enrolled")

    #4. Verify attestation signature
    # TODO: Replace stub with real impl
    payload = {
        "public_key_id": req.public_key_id,
        "nonce":         req.nonce,
        "timestamp":     req.timestamp,
        "firmware_hash": req.firmware_hash,
        "device_class":  req.device_class,
        "secure_boot":   req.secure_boot,
    }
    payload_bytes = json.dumps(payload, separators=(',', ':')).encode()

    if not verify_attestation(
            req.public_key_id,
            payload_bytes,
            bytes.fromhex(req.signature)):
        raise HTTPException(status_code=401,
                            detail="Attestation verification failed")

    # 5. Verify secure boot
    # TODO: Tighten for production mode
    # Currently set to warning for testing puposes
    if not req.secure_boot:
        print(f"WARNING: {req.public_key_id} "
              f"reports secure boot disabled")

    # 6. Assign manifest
    try:
        capabilities = ceiling_for_class(req.device_class)
    except ValueError as exc:
        raise HTTPException(
            status_code=400, detail="Unknown device class"
        ) from exc


    # 7. Sign manifest with gateway key
    manifest_version = int(time.time())
    manifest_data = json.dumps({
        "public_key_id":    req.public_key_id,
        "capabilities":     int(capabilities),
        "manifest_version": manifest_version,
    }, separators=(',', ':')).encode()

    gateway_sig = sign_manifest(manifest_data)

    # 8. Store enrollment in registry
    registry.enroll(public_key_id=req.public_key_id,
                    device_class=req.device_class,
                    capabilities=int(capabilities),
                    manifest_version=manifest_version,
                    firmware_hash=req.firmware_hash)

    return ManifestResponse(capabilities=int(capabilities),
                            manifest_version=manifest_version,
                            gateway_signature=gateway_sig.hex())

