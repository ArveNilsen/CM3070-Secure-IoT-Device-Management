"""
One-time setup: generates the gateway's ECDSA P-256 keypair used to sign issued
manifests. Run once! The private key is stored locally and must never be
distributed. The public key is printed for embedding into device firmware.

Usage:
    python3 generate_gateway_key.py
"""
from pathlib import Path

from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization

KEY_PATH = Path(__file__).parent / "gateway_private_key.pem"

def main():
    if KEY_PATH.exists():
        print(f"ERROR: {KEY_PATH} already exists. "
               "Refusing to overwrite an existing gateway key. "
               "Delete it manually if regeneration is needed.")
        return

    private_key = ec.generate_private_key(ec.SECP256R1())
    pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption(),
    )
    KEY_PATH.write_text(pem.decode())
    KEY_PATH.chmod(0o600) # owner read/write
    print(f"Gateway provate key written to {KEY_PATH}")

    # Print the public key point, as-is.
    public_numbers = private_key.public_key().public_numbers()
    x = public_numbers.x.to_bytes(32, byteorder='big')
    y = public_numbers.y.to_bytes(32, byteorder='big')
    raw_pub_hex = (x + y).hex()
    print("Gateway public key for embedding in firmware:")
    print(raw_pub_hex)



if __name__ == "__main__":
    main()

