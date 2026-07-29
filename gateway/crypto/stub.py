# from cryptography.hazmat.primitives.asymmetric import ec
# This is a stub version used for testing and development

REGISTERED_STUBS = {
    "stub-device-001",
    "stub-device-002",
}


def verify_attestation(
        public_key_id: str,
        payload: bytes,
        signature: bytes) -> bool:
    """
    Stub: accept any signature from a registered stub device.
    Logs that the real verification is bypassed.
    """
    if public_key_id not in REGISTERED_STUBS:
        print(f"STUB: Unknown device: "
              f"{public_key_id} - rejecting")
        return False

    print(f"STUB: Accepting attestation from "
          f"{public_key_id} without "
          f"cryptographic verification")
    return True


def sign_manifest(manifest_data: bytes) -> bytes:
    """
    Stub: return a fixed fake gateway signature.
    """
    import hashlib
    return hashlib.sha256(b"std-gateway-key" + manifest_data).digest()
