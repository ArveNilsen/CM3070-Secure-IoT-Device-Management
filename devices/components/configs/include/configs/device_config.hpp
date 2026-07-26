#pragma once

// Standard library includes
#include <string>
#include <span>
#include <expected>
#include <vector>

// ESP-IDF includes
#include "nvs.h"

namespace dev {

enum class ConfigError {
    NotFound,
    TypeMismatch,
    NVSFailure,
    NotEnrolled
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

class DeviceConfig {
public:

    /** The result type of the class.
     * void or error.
     */
    using result_type = std::expected<std::string, ConfigError>;

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

    // Lifecycle
    std::expected<void, ConfigError> init();

    // Network
    result_type wifi_ssid() const;
    result_type wifi_password() const;
    result_type gateway_host() const;

    // Identity
    result_type device_id() const;
    result_type public_key_id() const;
    std::string device_class() const; // from Kconfig, not NVS

    // Enrollment state
    bool is_enrolled() const;
    std::expected<void, ConfigError> set_enrolled(bool enrolled);

    // Manifest, written by enrollment component only
    std::expected<std::vector<uint8_t>, ConfigError> manifest() const;
    std::expected<void, ConfigError>
        store_manifest(std::span<const uint8_t> manifest,
                       std::span<const uint8_t> signature);

    // Firmware hash, written at provisioning only
    std::expected<std::vector<uint8_t>, ConfigError> firmware_hash() const;

private:
    DeviceConfig() = default;
    nvs_handle_t device_handle_;
    nvs_handle_t manifest_handle_;
    nvs_handle_t attestation_handle_;
};

} // namespace dev
