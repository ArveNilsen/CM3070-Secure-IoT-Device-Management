from fastapi.testclient import TestClient
from main import app
import time

client = TestClient(app)

def test_nonce_then_attest_succeds():
    resp = client.post("/enroll/nonce", json={
        "public_key_id": "stub-device-001",
        "device_class": "sensor"
    })
    assert resp.status_code == 200
    nonce = resp.json()["nonce"]

    attest_resp = client.post("/enroll/attest", json={
        "public_key_id": "stub-device-001",
        "nonce": nonce,
        "timestamp": int(time.time() * 1000),
        "firmware_hash": "aabbcc" * 10 + "aabb",
        "device_class": "sensor",
        "secure_boot": True,
        "signature": "00" * 64, # stub
    })
    assert attest_resp.status_code == 200


def test_double_enrollment_rejected():
    """NoDoubleEnrollment - Alloy, exercised via API"""
    resp = client.post("/enroll/nonce", json={
        "public_key_id": "stub-device-001",
        "device_class": "sensor"
    })
    assert resp.status_code == 200
    nonce = resp.json()["nonce"]

    attest_resp = client.post("/enroll/attest", json={
        "public_key_id": "stub-device-001",
        "nonce": nonce,
        "timestamp": int(time.time() * 1000),
        "firmware_hash": "aabbcc" * 10 + "aabb",
        "device_class": "sensor",
        "secure_boot": True,
        "signature": "00" * 64, # stub
    })
    assert attest_resp.status_code == 200

    resp = client.post("/enroll/nonce", json={
        "public_key_id": "stub-device-001",
        "device_class": "sensor"
    })
    assert resp.status_code == 409
    nonce = resp.json()["nonce"]

    attest_resp = client.post("/enroll/attest", json={
        "public_key_id": "stub-device-001",
        "nonce": nonce,
        "timestamp": int(time.time() * 1000),
        "firmware_hash": "aabbcc" * 10 + "aabb",
        "device_class": "sensor",
        "secure_boot": True,
        "signature": "00" * 64, # stub
    })
    assert attest_resp.status_code == 409


def test_stale_nonce_rejected():
    """
    Tests replay protection.
    Note: This is not modelled in any of the specs.
    """
    # TODO: Add to threat model discussion.
    resp = client.post("/enroll/nonce", json={
        "public_key_id": "stub-device-002",
        "device_class": "actuator"
    })
    nonce = resp.json()["nonce"]
    
    # Simulate nonce expiry by waiting or by manipulating
    # NonceStore TTL in test config
    time.sleep(61) # exceeds NONCE_TTL_SECONDS TODO: remove hardcoded value

    attest_resp = client.post("/enroll/attest", json={
        "public_key_id": "stub-device-002",
        "nonce": nonce,
        "timestamp": int(time.time() * 1000),
        "firmware_hash": "aabbcc" * 10 + "aabb",
        "device_class": "sensor",
        "secure_boot": True,
        "signature": "00" * 64, # stub
    })

def test_invalid_signature_rejected():
    """
    This test is only meaningful if stub mode is disabled.
    """
    resp = client.post("/enroll/nonce", json={
        "public_key_id": "esp32-001",
        "device_class":  "sensor",
    })
    nonce = resp.json()["nonce"]

    attest_resp = client.post("/enroll/attest", json={
        "public_key_id": "esp32-001",
        "nonce": nonce,
        "timestamp": int(time.time() * 1000),
        "firmware_hash": "gg" # TODO: Add the correct hash here
        "device_class": "sensor",
        "secure_boot": True,
        "signature": "00" * 64, # This is deliberately invalid
    })
    assert attest_resp.status_code == 401

