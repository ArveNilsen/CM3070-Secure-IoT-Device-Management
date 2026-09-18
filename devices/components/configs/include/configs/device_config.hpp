#pragma once

// Standard library includes
#include <string>
#include <span>
#include <expected>
#include <vector>
#include <cstdint>

// ESP-IDF includes
#include "nvs.h"
#include "esp_err.h"

namespace dev {

enum class ConfigError {
    NotFound,
    TypeMismatch,
    NVSFailure,
    NotEnrolled,
		NotInitialized
};

constexpr esp_err_t to_esp_err(ConfigError e) noexcept 
{
    switch (e) {
        case ConfigError::NotFound:     return ESP_ERR_NOT_FOUND;
        case ConfigError::TypeMismatch: return ESP_ERR_INVALID_ARG;
        case ConfigError::NVSFailure:   return ESP_FAIL;
        case ConfigError::NotEnrolled:  return ESP_ERR_INVALID_STATE;
    }
    return ESP_FAIL; // unreachable
}

/**
 * @brief Persistent device configuration.
 *
 * Three NVS namespaces for distinct access patterns:
 * - "device_cfg" runtime config.
 * - "manifest" enrollement-issued capability manifest.
 * - "attestation" firmware hash and identity material.
 */
class DeviceConfig {
public:

    /** The result type of the class.
     * void or error.
     */
    using string_result = std::expected<std::string, ConfigError>;
    using bytes_result  = std::expected<std::vector<uint8_t>, ConfigError>;

    /**
     * @breif Copy constructor deleted.
     */
    DeviceConfig(const DeviceConfig&)               = delete;

    /**
     * @breif Copy assignment operator deleted.
     */
    DeviceConfig& operator=(const DeviceConfig&)    = delete;

    /**
     * @brief Get singleton
     */
    static DeviceConfig& instance();

    // Lifecycle. Must be called before other methods.
    std::expected<void, ConfigError> init();

    // Network
    result_type wifi_ssid() const;
    result_type wifi_password() const;
    result_type gateway_host() const;

    // Identity
    result_type device_id() const;
    result_type public_key_id() const;
    std::string device_class() const; // Kconfig

    // Enrollment state
    bool is_enrolled() const;
    std::expected<void, ConfigError> set_enrolled(bool enrolled);

		/**
		 * @brief Store an enrollment manifest
		 *
		 * @param manifest The manifest bytes as received from the gateway.
		 * As JSON, not reserialized.
		 * 
		 * @param signature The gateway's signature as bytes.
		 */
    bytes_result manifest() const;
    std::expected<void, ConfigError>
        store_manifest(std::span<const uint8_t> manifest,
                       std::span<const uint8_t> signature);

    // Firmware hash, written at provisioning only
    bytes_result firmware_hash() const;

		/**
		 * @brief Parsed capability ceiling from the store manifest.
		 *
		 * Convenience accessor that parses the manifest JSON once.
		 */
		std::expected<uint32_t, ConfigError> capability_ceiling() const;

private:
    DeviceConfig() = default;

		std::expected<void, ConfigError> ensure_initialized() const;

																					 // Three handles by design
    nvs_handle_t device_handle_				= 0; // runtime config
    nvs_handle_t manifest_handle_			= 0; // enrollment write-only
    nvs_handle_t attestation_handle_	= 0; // provision write-only
		bool initialized_ = false;
};

} // namespace dev
