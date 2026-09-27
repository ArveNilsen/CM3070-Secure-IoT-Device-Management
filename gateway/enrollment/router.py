import json

#import hmac
import time
import subprocess

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, ValidationError

from enrollment.attestation import sign_manifest, verify_attestation
from policy.manifests import ceiling_for_class
from registry.store import DeviceRegistry
from enforcement.backend import NullBackend

from .nonce_store import NonceStore

registry: DeviceRegistry = None

router      = APIRouter(prefix="/enroll")
nonce_store = NonceStore()
backend     = NullBackend()

# SHA-256 of the released firmware per device class
# TODO: Add the actual hashes
# TODO: Move to appropriate location
EXPECTED_FIRMWARE_HASHES: dict[str, str] = {
    "sensor": "d89ee89c7608f846fe95c0cae857156dd5aa55c8939129cb95453731d90dd72b",
    "actuator": "ddeeff...",
}

# --- Request / Response models ---

class NonceRequest(BaseModel):
    public_key_id: str
    device_class: str


class NonceResponse(BaseModel):
    nonce: str # hex-encoded
    timestamp: int


class ManifestResponse(BaseModel):
    manifest: str
    manifest_version: int
    gateway_signature: str # hex-encoded


class AttestationEnvelope(BaseModel):
    payload: str
    signature: str


class AttestationPayload(BaseModel):
    """
    Schema for the field inside 'payload'
    """
    public_key_id : str
    nonce: str
    timestamp: int
    firmware_hash: str
    device_class: str
    secure_boot: bool
    mac_address: str

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
async def submit_attestation(envelope: AttestationEnvelope):
    """
    Phase 2: device submits signed attestation evidence. Gateway verifies and
    issues manifest.

    Verifies against 'envelope.payload' as bytes with no re-serialisation.
    """

    # 1. Verify nonce is valid and consume it
    # This requires first verifying the signature and looking up
    # the public_ley
    payload_bytes = envelope.payload.encode("utf-8")

    try:
        signature = bytes.fromhex(envelope.signature)
    except ValueError as exc:
        raise HTTPException(
            status_code=400, detail="Malformed signature encoding"
        ) from exc

    # Look up public key
    try:
        fields = json.loads(envelope.payload)
        payload = AttestationPayload(**fields)
    except json.JSONDecodeError as exc:
        print(f"[ATTEST] JSON decode failed: {exc}")
        print(f"[ATTEST] Raw payload was: {envelope.payload!r}")
        raise HTTPException(status_code=400,
            detail="Malformed attestation payload (JSON)") from exc
    except ValidationError as exc:
        print(f"[ATTEST] Pydantic validation failed: {exc}")
        raise HTTPException(status_code=400,
            detail="Malformed attestation payload (schema)") from exc
    """
    try:
        fields = json.loads(envelope.payload)
        payload = AttestationPayload(**fields)
    except (json.JSONDecodeError, ValidationError) as exc:
        raise HTTPException(
            status_code=400, detail="Malformed attestation payload"
        ) from exc"""

    if not verify_attestation(payload.public_key_id, payload_bytes, signature):
        raise HTTPException(
            status_code=401, detail="Attestation verification failed")

    try:
        presented_nonce = bytes.fromhex(payload.nonce)
    except ValueError as exc:
        raise HTTPException(
            status_code=400, detail="Malformed nonce"
        ) from exc

    if not nonce_store.consume(payload.public_key_id, presented_nonce):
        raise HTTPException(status_code=401, detail="Invalid or expired nonce")

    # 2. Verify timestamp freshness
    # TODO: Consider NTP or some TTL mechanism
    """
    age = abs(time.time() - payload.timestamp / 1000)
    if age > 90: # TODO: Remove hardcoded value
        raise HTTPException(status_code=401, detail="Timestamp too stale")
    """

    # 3. Verify device not already enrolled
    if registry.is_enrolled(payload.public_key_id):
        print(f"[ENROLLMENT] STATE MISMATCH: device '{payload.public_key_id}' "
              "attempted enrollment but is already present in the registry. "
              "Remove the device from the registry or set up the system anew.")
        raise HTTPException(status_code=409, detail="Device already enrolled")

    #4. Verify firmware hash
    expected = EXPECTED_FIRMWARE_HASHES.get(payload.device_class)
    if expected and payload.firmware_hash.lower() != expected.lower():
        raise HTTPException(
            status_code=401, detail="Firmware hash mismatch")

    # 5. Verify secure boot
    # TODO: Tighten for production mode
    # Currently set to warning for testing puposes
    if not payload.secure_boot:
        print(f"WARNING: {payload.public_key_id} "
              f"reports secure boot disabled")

    # 6. Assign manifest
    try:
        capabilities = ceiling_for_class(payload.device_class)
    except ValueError as exc:
        raise HTTPException(
            status_code=400, detail="Unknown device class"
        ) from exc


    # 7. Sign manifest with gateway key
    manifest_version = int(time.time())
    manifest_data = json.dumps({
        "public_key_id":    payload.public_key_id,
        "capabilities":     int(capabilities),
        "manifest_version": manifest_version,
    }, separators=(',', ':')).encode()

    gateway_sig = sign_manifest(manifest_data)

    # 8. Store enrollment in registry
    registry.enroll(
        public_key_id=payload.public_key_id,
        device_class=payload.device_class,
        capabilities=int(capabilities),
        manifest_version=manifest_version,
        firmware_hash=payload.firmware_hash,
        mac_address=payload.mac_address)

    try:
        backend.apply_capabilities(payload.mac_address, int(capabilities))
    except subprocess.CalledProcessError as exc:
        print("[WARN] Failed to apply OVS capabilities for "
              f"'{payload.public_key_id}': {exc}. Run reconcile().")

    return ManifestResponse(
        manifest=manifest_data.decode(),
        manifest_version=manifest_version,
        gateway_signature=gateway_sig.hex())

