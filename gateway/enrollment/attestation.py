from __future__ import annotations

import hashlib
import os
from pathlib import Path

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature
from registry.trust_store import TrustStore


def raw_to_der(raw_sig: bytes) -> bytes:
    """
    TODO: Add to report, implementation section.
    Embedded use R||S, server uses DER

    Must be applied before calling pub_key.verify()

    The ATEC608 (via cryptoauthlib's atcab_sign) produces a raw 64-byte R||S
    signature. This function converts to DER.
    """
    if len(raw_sig) != 64:
        raise ValueError(
            f"Expected 64-byte raw R||S signature, got {len(raw_sig)} bytes")

    r = int.from_bytes(raw_sig[:32], byteorder='big')
    s = int.from_bytes(raw_sig[32:], byteorder='big')
    return encode_dss_signature(r, s)

# ---
# Stub verification
# ---

STUB_DEVICES: dict[str, bytes] = {
    "stub-device-001": b"",
    "stub-device-002": b"",
}


def _verify_stub(public_key_id: str, payload: bytes, signature: bytes) -> bool:
    """
    Accept any well-formed request from a registered stub device ID.
    Logs that verification is bypassed.
    """
    if public_key_id not in STUB_DEVICES:
        print(f"[STUB ATTESTATION] REJECTED: unknown device '{public_key_id}'")
        return False

    print(f"[STUB ATTESTATION] WARNING: "
          f"accepting '{public_key_id}' without cryptographic verification. "
          "Replace before production.")
    return True


# ---
# ECDSA verification (P-256), backed by the trust store
# populated via register_device.py
# ---

_trust_store: TrustStore | None = None


def _get_trust_store() -> TrustStore:
    """
    Avoids opening the trust store in stub mode.
    """
    global _trust_store
    if _trust_store is None:
        _trust_store = TrustStore(
            str(Path(__file__).parent.parent / "trust_store.db"))
    return _trust_store


def _verify_ecdsa(public_key_id: str, payload: bytes,
                  signature: bytes) -> bool:
    row = _get_trust_store().get(public_key_id)
    if row is None:
        print(f"[ATTESTATION] REJECTED: "
              f"'{public_key_id}' is not a registered device")
        return False

    try:
        raw_pub = bytes.fromhex(row["public_key_id"])
    except ValueError:
        print(f"[ATTESTATION] Malformed stored public key "
              f"for '{public_key_id}'")
        return False

    if len(raw_pub) != 64:
        print(f"[ATTESTATION] REJECTED: '{public_key_id}' "
              "has a malformed public key "
              f"(expected 64 bytes, got {len(raw_pub)})")
        return False

    try:
        der_signature = raw_to_der(signature)
    except ValueError as e:
        print(f"[ATTESTATION] Malformed signature for "
              f"'{public_key_id}': {e}")
        return False

    try:
        pub_key = ec.EllipticCurvePublicKey.from_encoded_point(
            ec.SECP256R1(), b'\x04' + raw_pub) # uncompressed
        pub_key.verify(der_signature, payload, ec.ECDSA(hashes.SHA256()))
        return True
    except InvalidSignature:
        print("[ATTESTATION] REJECTED: invalid signature "
              f"for '{public_key_id}'")
        return False
    except Exception as e:
        print("[ATTESTATION] Verification error for "
              f"'{public_key_id}': {e}")
        return False


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
