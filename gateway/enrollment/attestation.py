from __future__ import annotations

import hashlib
import os

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature
from cryptography.exceptions import InvalidSignature

def raw_to_der(raw_sig: bytes) -> bytes:
    """
    TODO: Add to report, implementation section. 
    Embedded use R||S, server uses DER

    Must be applied before calling pub_key.verify()
    """
    r = int.from_bytes(raw_sig[:32], byteorder='big')
    s = int.from_bytes(raw_sig[32:], byteorder='big')
    return encode_dss_signature(r, s)

# ---
# Registered stub devices
# Must match CONFIG values on device side
# ---

STUB_DEVICES: dict[str, bytes] = {
    "stub-device-001": b"",
    "stub-device-002": b"",
}

# ---
# Stub verification
# ---

def _verify_stub(public_key_id: str, payload: bytes,
                 signature: bytes) -> bool:
    """
    Accept any well-formed request from a registered stub device ID.
    Logs that verification is bypassed.
    """
    if public_key_id not in STUB_DEVICES:
        print(f"[STUB ATTESTATION] REJECTED: "
              f"unknown device '{public_key_id}'")
        return False

    print(f"[STUB ATTESTATION] WARNING: "
          f"accepting '{public_key_id}' without "
          f"cryptographic verification. "
          f"Replace before production.")
    return True


# ---
# ECDSA verification (P-256)
# ---

REGISTERED_DEVICES: dict[str, bytes] = {
    "esp32-001": bytes.fromhex("AABBCCDD..."), # TODO: Add provisioning output.
}

def _verify_ecdsa(public_key_id: str,
                  payload: bytes,
                  signature: bytes) -> bool:
    raw_pub = REGISTERED_DEVICES.get(public_key_id)

    if not raw_pub:
        return False

    try:
        pub_key = ec.EllipticCurvePublicKey.from_encoded_point(
            ec.SECP256R1(), b'\x04' + raw_pub) # uncompressed
        pub_key.verify(signature, payload, ec.ECDSA(hashes.SHA256()))
        return True

    except InvalidSignature as e:
        print(f"Invalid signature error: {e}")
        return False

    except Exception as e:
        print(f"Verification error: {e}")
        return False


def _verify_ecdsa(public_key_id: str, payload: bytes, signature: bytes) -> bool:
    raise NotImplementedError


# ---
# Public interface
# ---

STUB_MODE: bool = bool(
    os.environ.get("GATEWAY_STUB_MODE", "1") != "0")


def verify_attestation(public_key_id: str,
                       payload: bytes,
                       signature: bytes) -> bool:
    """
    Verify attestation evidence from a device.

    In stub mode: accept any signature from a registered device ID.

    In production mode: verifies ECDSA P-256 signature against
    registered public key.

    The payload must be in the canonical JSON serialisation of the
    attestation fields, matching what the device signed.
    """
    if STUB_MODE:
        return _verify_stub(public_key_id, payload, signature)

    return _verify_ecdsa(public_key_id, payload, signature)


def sign_manifest(manifest_data: bytes) -> bytes:
    """
    Sign a manifest with the gateway key.

    Stub: returns SHA-256 of data with a fixed prefix. Not a real signature.
    """
    if STUB_MODE:
        stub_key = b"stub-gateway-signing-key"
        return hashlib.sha256(stub_key + manifest_data).digest()

    # Load gateway private key and sign with ECDSA P-256
    raise RuntimeError("Production manifest signing not yet implemented")
