#pragma once

#include <string>
#include <span>
#include <expected>

namespace dev {

enum class ConfigError {
    NotFound,
    TypeMismatch,
    NVSFailure,
    NotEnrolled
};

class DeviceConfig {
    static DeviceConfig& instance();

    // Lifecycle
    std::expected<void, ConfigError> init();

    // Network
    std::expected<std::string, ConfigError> wifi_ssid() const;
    std::expected<std::string, ConfigError> wifi_password const;
    std::expected<std::string, ConfigError> gateway_host const;

    // Identity
    std::expected<std::string, ConfigError> device_id() const;
    std::expected<std::string, ConfigError> public_key_id() const;
    std::string device_class() const; // from Kconfig, not NVS

    // Enrollment state
    bool is_enrolled() const;
    std::expected<void, ConfigError>
        set_enrolled(bool enrolled);

    // Manifest, written by enrollment component only
    std::expected<std::vector<uint8_t>, ConfigError>
        manifest() const;
    std::expected<void, ConfigError>
        store_manifest(std::span<const uint8_t> manifest,
                       std::span<const uint_8t> signature);

    // Firmware hash, written at provisioning only
    std::expected<std::vector<uint8_t>, ConfigError>
        firmware_hash() const;

private:
    DeviceConfig() = default;
    nvs_handle_t device_handle_;
    nvs_handle_t manifest_handle_;
    nvs_handle_t attestation_handle_;
};

} // namespace dev
