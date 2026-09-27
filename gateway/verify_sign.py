from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature
from cryptography.exceptions import InvalidSignature

payload_without_mac = b'{"public_key_id":"esp32-001","nonce":"4325a122b87f81fd3696d0ccd6c86d696818c2056cb1a4d7105eeedaacf224b6","timestamp":2984,"firmware_hash":"eb2f73efb6ed940139c827e65277fd159a3ba160e260a2354fd0b58c4ec5d7e5","device_class":"sensor","secure_boot":false}'

# same verify logic as before, but against this shorter payload
payload = b'{"public_key_id":"esp32-001","nonce":"4325a122b87f81fd3696d0ccd6c86d696818c2056cb1a4d7105eeedaacf224b6","timestamp":2984,"firmware_hash":"eb2f73efb6ed940139c827e65277fd159a3ba160e260a2354fd0b58c4ec5d7e5","device_class":"sensor","secure_boot":false,"mac_address":"38:18:2B:8B:F9:48"}'

raw_signature_hex = "582b84224ba387d72f87703d12065b6ab4d63df6cf107c03599a94ec23c275fd9ace10e40a98545e96e454ec46e662f4ab8c471558778be7fbbc4338df7157b1"

public_key_hex = "166a1d36385fe833921fe578935ac63b74acd33ed9ad1ce5e8d285c252212201969488c1e85777ec614fe97b222b592a64a7cba41afdfb42dc83273716773d9d"

raw_sig = bytes.fromhex(raw_signature_hex)
print(f"Raw signature length: {len(raw_sig)}")

r = int.from_bytes(raw_sig[:32], byteorder='big')
s = int.from_bytes(raw_sig[32:], byteorder='big')
der_sig = encode_dss_signature(r, s)

raw_pub = bytes.fromhex(public_key_hex)
print(f"Public key length: {len(raw_pub)}")

pub_key = ec.EllipticCurvePublicKey.from_encoded_point(
    ec.SECP256R1(), b'\x04' + raw_pub)

try:
    pub_key.verify(der_sig, payload_without_mac, ec.ECDSA(hashes.SHA256()))
    print("VERIFICATION SUCCEEDED")
except InvalidSignature:
    print("VERIFICATION FAILED")
