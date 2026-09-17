// Project includes
#include "enrollment/enrollment.hpp"
#include "enrollment/hex_util.hpp"

// ESP-IDF includes
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"

// Third-party includes
#include <cJSON.h>
#include "mbedtls/sha256.h"

// Standard library includes
#include <string>

static const char* TAG = "enrollment";

// Hardcoded stub values, to be replaced with HardwareIdentity
static constexpr char STUB_PUBLIC_KEY_ID[] = "stub-device-001";
static constexpr char STUB_FIRMWARE_HASH[] = 
    "aabbccddaabbccddaabbccddaabbccdd"
    "aabbccddaabbccddaabbccddaabbccdd";

using dev::EnrollmentService;
using dev::EnrollmentResult;
using dev::EnrollmentError;

EnrollmentService::EnrollmentService( DeviceConfig& config,
    HardwareIdentity& identity)
    : config_{config}, identity_{identity} {}

std::expected<EnrollmentResult, EnrollmentError> EnrollmentService::enroll()
{
    if (is_enrolled()) {
        ESP_LOGI(TAG, "Already enrolled, skipping");
        return std::unexpected(EnrollmentError::AlreadyEnrolled);
    }

    ESP_LOGI(TAG, "Starting enrollment...");

    // Phase 1: get nonce
    auto nonce = request_nonce();
    if (!nonce) 
        return std::unexpected(nonce.error());

    // Phase 2: build and sign attestation
    auto evidence = build_attestation(*nonce);
    if (!evidence)
        return std::unexpected(evidence.error());

    // Phase 3: submit and receive manifest
    auto result = submit_attestation(*evidence);
    if (!result)
        return std::unexpected(result.error());

    // Phase 4: verify gateway signature over manifest
    auto verified = verify_manifest(*result);
    if (!verified)
        return std::unexpected(verified.error());

    ESP_LOGI(TAG, "Enrollment complete. "
                  "Manifest version: %" PRIu32, result->manifest_version);
    return *result;
}

bool EnrollmentService::is_enrolled() const
{
    return false;
}

std::expected<std::vector<uint8_t>, EnrollmentError> 
EnrollmentService::request_nonce()
{
    // Build request body
    cJSON* body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "public_key_id", STUB_PUBLIC_KEY_ID);
    cJSON_AddStringToObject(body, "device_class", 
        config_.device_class().c_str());

    char* body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    // Configure HTTP client
    std::string url = "http://"
        + config_.gateway_host().value_or("192.168.1.1")
        + ":"
        + std::to_string(CONFIG_GATEWAY_PORT)
        + "/enroll/nonce";

    esp_http_client_config_t cfg = {};
    cfg.url        = url.c_str();
    cfg.method     = HTTP_METHOD_POST;
    cfg.timeout_ms = CONFIG_NONCE_TIMEOUT_MS;

    auto client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body_str, strlen(body_str));

    // Perform request
    esp_err_t err = esp_http_client_perform(client);
    free(body_str);

    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return std::unexpected(EnrollmentError::NetworkFailure);
    }

    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        esp_http_client_cleanup(client);
        ESP_LOGE(TAG, "Nonce request failed: HTTP %d", status);
        return std::unexpected(EnrollmentError::GatewayRejected);
    }

    // Read response body
    int content_len = esp_http_client_get_content_length(client);
    std::vector<char> response_buf(content_len + 1, 0);
    esp_http_client_read_response(client, response_buf.data(), content_len);
    esp_http_client_cleanup(client);

    // Parse nonce from response
    cJSON* response = cJSON_Parse(response_buf.data());
    if (!response)
        return std::unexpected(EnrollmentError::GatewayRejected);

    cJSON* nonce_item = cJSON_GetObjectItem(response, "nonce");
    if (!cJSON_IsString(nonce_item)) {
        cJSON_Delete(response);
        return std::unexpected(EnrollmentError::GatewayRejected);
    }

    // Decode hex nonce to bytes
    std::string nonce_hex = nonce_item->valuestring;
    cJSON_Delete(response);

    std::vector<uint8_t> nonce_bytes = hex_decode(nonce_hex);

    ESP_LOGI(TAG, "Nonce received: %s", nonce_hex.c_str());
    return nonce_bytes;
}

std::expected<std::vector<uint8_t>, EnrollmentError>
EnrollmentService::build_attestation(std::span<const uint8_t> nonce)
{
/*
 * Attestation evidence as serialisable structure.
AttestationEvidence {
    public_key_id   : string
    nonce           : bytes
    timestamp       : uint64
    firmware_hash   : bytes[32]
    device_class    : string
    secure_boot     : bool
    signature       : bytes[64]  ← ECDSA over all fields above
}
Add CBOR as dependency
*/
    int64_t timestamp = esp_timer_get_time() / 1000;

    // Build the payload that will be signed.
    // Precondition:
    // Field order is canonical, gateway must hash fields in the same order.
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "public_key_id", STUB_PUBLIC_KEY_ID);
    cJSON_AddStringToObject(payload, "nonce", hex_encode(nonce).c_str());
    cJSON_AddNumberToObject(payload, "timestamp", (double)timestamp);
    cJSON_AddStringToObject(payload, "firmware_hash", STUB_FIRMWARE_HASH);
    cJSON_AddStringToObject(payload, "device_class", 
        config_.device_class().c_str());
    cJSON_AddBoolToObject(payload, "secure_boot", 
        identity_.secure_boot_enabled());

    // Serialise payload for signing
    char* payload_str = cJSON_PrintUnformatted(payload);
    std::string payload_canonical(payload_str);
    free(payload_str);

    std::array<uint8_t, 32> digest;
    mbedtls_sha256(reinterpret_cast<const uint8_t*>(payload_canonical.data()),
        payload_canonical.size(), digest.data(), 0/* 0 = SHA-256 */);

    // Sign with stub key TODO: replace with ATECC608A
    auto sig_result = identity_.sign(CONFIG_ATTESTATION_KEY_SLOT, digest);

    if (!sig_result) {
        cJSON_Delete(payload);
        return std::unexpected(EnrollmentError::AttestationFailed);
    }
    
    // Add signature to payload for transmission
    cJSON_AddStringToObject(payload, "signature", 
        hex_encode(*sig_result).c_str());

    char *evidence_str = cJSON_PrintUnformatted(payload);
    cJSON_Delete(payload);

    std::vector<uint8_t> evidence(evidence_str, 
        evidence_str + strlen(evidence_str));
    free(evidence_str);

    return evidence;
}

// Phase 3: submit evidence, receive manifest
std::expected<EnrollmentResult, EnrollmentError>
EnrollmentService::submit_attestation(std::span<const uint8_t> evidence)
{
    return {};
}

// Phase 4: verify gateway signature over manifest
std::expected<void, EnrollmentError>
EnrollmentService::verify_manifest(const EnrollmentResult& result)
{
    return {};
}

