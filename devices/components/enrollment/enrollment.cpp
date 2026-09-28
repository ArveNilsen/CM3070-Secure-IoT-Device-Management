// Project includes
#include "enrollment/enrollment.hpp"
#include "enrollment/hex_util.hpp"
#include "json_util/json_value.hpp"

// ESP-IDF includes
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"

// Third-party includes
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
    return config_.is_enrolled();
}

std::expected<std::vector<uint8_t>, EnrollmentError> 
EnrollmentService::request_nonce()
{
		auto pkid = config_.public_key_id();
		if (!pkid) {
				ESP_LOGE(TAG, "No public_key_id in config");
				return std::unexpected(EnrollmentError::AttestationFailed);
		}

		auto host = config_.gateway_host();
		if (!host) {
				ESP_LOGE(TAG, "No gateway host in config");
				return std::unexpected(EnrollmentError::NotConfigured);
		}

    // Build request body
		JsonValue payload = JsonValue::object();
		payload.add_string("public_key_id", pkid->c_str());
    payload.add_string( "device_class", config_.device_class().c_str());

		std::string body_owned = payload.dump_unformatted();

    // Configure HTTP client
    std::string url = "http://" + *host + ":"
        + std::to_string(CONFIG_GATEWAY_PORT) + "/enroll/nonce";
		ESP_LOGI(TAG, "Requesting nonce: %s", url.c_str());

    esp_http_client_config_t cfg = {};
    cfg.url        = url.c_str();
    cfg.method     = HTTP_METHOD_POST;
    cfg.timeout_ms = CONFIG_NONCE_TIMEOUT_MS;

    auto client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
		
		esp_err_t err = esp_http_client_open(client, body_owned.size());
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return std::unexpected(EnrollmentError::NetworkFailure);
    }

		int written = esp_http_client_write(client, body_owned.data(), body_owned.size());
		if (written < 0) {
				esp_http_client_close(client);
				esp_http_client_cleanup(client);
				return std::unexpected(EnrollmentError::NetworkFailure);
		}

		int content_length = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
		
    // Read response body
    std::vector<char> response_buf(content_length + 1, 0);
    int read_len = esp_http_client_read_response(
				client, response_buf.data(), content_length);

		esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (status != 200) {
        ESP_LOGE(TAG, "Nonce request failed: HTTP %d, body: %s", 
						status, response_buf.data());
        return std::unexpected(EnrollmentError::GatewayRejected);
    }

		ESP_LOGI(TAG, "Nonce response (%d bytes): %s", content_length, response_buf.data());

    // Parse nonce from response
		JsonValue response = JsonValue::parse(response_buf.data());
    if (!response) {
        return std::unexpected(EnrollmentError::GatewayRejected);
		}

		auto nonce_item = response.get_string("nonce");
		if (!nonce_item) {
				ESP_LOGE(TAG, "Response missing 'nonce' string field");
        return std::unexpected(EnrollmentError::GatewayRejected);
		}

    std::vector<uint8_t> nonce_bytes = hex_decode(*nonce_item);
    ESP_LOGI(TAG, "Nonce received: %s", (*nonce_item).c_str());

    return nonce_bytes;
}

std::expected<std::vector<uint8_t>, EnrollmentError>
EnrollmentService::build_attestation(std::span<const uint8_t> nonce)
{
		auto pkid = config_.public_key_id();
		if (!pkid) {
				ESP_LOGE(TAG, "No public_key_id in config");
				return std::unexpected(EnrollmentError::AttestationFailed);
		}
		
		auto fw_hash = identity_.firmware_hash(CONFIG_FIRMWARE_HASH_SLOT);
		if (!fw_hash) {
				ESP_LOGE(TAG, "CONFIG_FIRMWARE_HASH_SLOT: %d", CONFIG_FIRMWARE_HASH_SLOT);
				ESP_LOGE(TAG, "Failed to read firmware hash from hardware identity");
				return std::unexpected(EnrollmentError::AttestationFailed);
		}

		auto mac_addr = config_.mac_addr();
		if (!mac_addr) {
				ESP_LOGE(TAG, "Failed to get MAC address from config");
				return std::unexpected(EnrollmentError::AttestationFailed);
		}

    int64_t timestamp = esp_timer_get_time() / 1000;

    // Build the payload that will be signed.
    // Precondition:
    // Field order is canonical, gateway must hash fields in the same order.
		JsonValue payload = JsonValue::object();
    payload.add_string("public_key_id", *pkid);
    payload.add_string("nonce", hex_encode(nonce));
    payload.add_number("timestamp", (double)timestamp);
    payload.add_string("firmware_hash", hex_encode(*fw_hash));
    payload.add_string("device_class", config_.device_class());
    payload.add_bool("secure_boot", identity_.secure_boot_enabled());
		payload.add_string("mac_address", *mac_addr);

    // Serialise payload for signing
		std::string payload_canonical = payload.dump_unformatted();

		ESP_LOGI(TAG, "Attestation payload: %s", payload_canonical.c_str());

    std::array<uint8_t, 32> digest;
    mbedtls_sha256(reinterpret_cast<const uint8_t*>(payload_canonical.data()),
        payload_canonical.size(), digest.data(), 0/* 0 = SHA-256 */);

    auto sig_result = identity_.sign(CONFIG_ATTESTATION_KEY_SLOT, digest);
    if (!sig_result) {
				ESP_LOGE(TAG, "Signing failed");
        return std::unexpected(EnrollmentError::AttestationFailed);
		}
		ESP_LOGI(TAG, "Raw signature (64 bytes): %s", 
				hex_encode(*sig_result).c_str());
    
    // Add signature to payload for transmission
		JsonValue envelope = JsonValue::object();
    envelope.add_string("payload", payload_canonical);
    envelope.add_string("signature", hex_encode(*sig_result));

		std::string evidence_str = envelope.dump_unformatted();
    return std::vector<uint8_t> {evidence_str.begin(), evidence_str.end()};
}

// Phase 3: submit evidence, receive manifest
std::expected<EnrollmentResult, EnrollmentError>
EnrollmentService::submit_attestation(std::span<const uint8_t> evidence)
{
		std::string url = "http://"
			+ config_.gateway_host().value_or("0.0.0.0")
			+ ":" + std::to_string(CONFIG_GATEWAY_PORT)
			+ "/enroll/attest";

		esp_http_client_config_t cfg = {};
		cfg.url					= url.c_str();
		cfg.method			= HTTP_METHOD_POST;
		cfg.timeout_ms  = CONFIG_NONCE_TIMEOUT_MS;

		auto client = esp_http_client_init(&cfg);
		esp_http_client_set_header(client, "Content-Type", "application/json");

		esp_err_t err = esp_http_client_open(client, evidence.size());
		if (err != ESP_OK) {
			esp_http_client_cleanup(client);
			return std::unexpected(EnrollmentError::NetworkFailure);
		}

		int written = esp_http_client_write(
				client, reinterpret_cast<const char*>(evidence.data()), evidence.size());
		if (written < 0) {
			esp_http_client_close(client);
			esp_http_client_cleanup(client);
			return std::unexpected(EnrollmentError::NetworkFailure);
		}

		int content_length = esp_http_client_fetch_headers(client);
		int status = esp_http_client_get_status_code(client);

		std::vector<char> response_buf(content_length + 1, 0);
		int read_len = esp_http_client_read_response(
				client, response_buf.data(), content_length);

		ESP_LOGI(TAG, "Requested %d bytes, read %d bytes",
				content_length, read_len);

		esp_http_client_close(client);
		esp_http_client_cleanup(client);

		if (status == 409) {
				ESP_LOGE(TAG, "ENROLLMENT STATE MISMATCH\n"
						"  The gateway reports this device as ALREADY enrolled, but this "
						"  device has no record of it.\n Refusing to re-enroll, remove the "
						"  device from the registry or set up the system from scratch.");
				return std::unexpected(EnrollmentError::StateMismatch);
		}

		if (status != 200) {
			ESP_LOGE(TAG, "Attestation rejected: HTTP %d, body: %s",
					status, response_buf.data());
			return std::unexpected(EnrollmentError::GatewayRejected);
		}

		ESP_LOGI(TAG, "Manifest response (%d bytes): %s",
			read_len, response_buf.data());

		JsonValue response = JsonValue::parse(response_buf.data());
		if (!response) {
				ESP_LOGE(TAG, "Failed to parse manifest response");
				return std::unexpected(EnrollmentError::ManifestInvalid);
		}

		auto manifest_item = response.get_string("manifest");
		if (!manifest_item) {
				ESP_LOGE(TAG, "Response is missing manifest.");
				return std::unexpected(EnrollmentError::ManifestInvalid);
		}
		auto sig_item = response.get_string("gateway_signature");
		if (!sig_item) {
				ESP_LOGE(TAG, "Response is missing signature");
				return std::unexpected(EnrollmentError::ManifestInvalid);
		}

		EnrollmentResult result;
		std::string manifest_str = *manifest_item;
		ESP_LOGI(TAG, "manifest_str: %s", manifest_str.c_str());
		result.manifest.assign(manifest_str.begin(), manifest_str.end());
		result.gateway_signature = hex_decode(*sig_item);

    return result;
}

// Phase 4: verify gateway signature over manifest
std::expected<void, EnrollmentError>
EnrollmentService::verify_manifest(const EnrollmentResult& result)
{
		JsonValue manifest = JsonValue::parse(
				{result.manifest.begin(), result.manifest.end()});
		if (!manifest) {
				ESP_LOGE(TAG, "Manifest is not valid JSON");
				return std::unexpected(EnrollmentError::ManifestInvalid);	
		}
		
		auto pkid = manifest.get_string("public_key_id");
		if (!pkid) {
				ESP_LOGE(TAG, "Manifest missing required public key field");
		}
		auto caps = manifest.get_number("capabilities");	
		if (!caps) {
				ESP_LOGE(TAG, "Manifest missing required capabilities field");
		}
		auto ver  = manifest.get_number("manifest_version");	
		if (!ver) {
				ESP_LOGE(TAG, "Manifest missing required version field");
		}
		if (!pkid || !caps || !ver) {
				ESP_LOGE(TAG, "Manifest missing required fields");
				return std::unexpected(EnrollmentError::ManifestInvalid);
		}

		if (result.gateway_signature.empty()) {
				ESP_LOGE(TAG, "Manifest carries no signature");
				return std::unexpected(EnrollmentError::SignatureInvalid);
		}

		// Gateway key must be embedded in firmware
		ESP_LOGW(TAG, "Gateway signature present. STUB impl");

		return {};
}

