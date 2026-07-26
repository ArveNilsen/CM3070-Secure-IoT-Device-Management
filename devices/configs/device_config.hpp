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
public:

    /** The result type of the class.
     * void or error.
     */
    using result_type = std::expected<std::string, ConfigError>;

    static DeviceConfig& instance();

    // Lifecycle
    std::expected<void, ConfigError> init();

    // Network
    result_type wifi_ssid() const;
    result_type wifi_password const;
    result_type gateway_host const;

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
                       std::span<const uint_8t> signature);

    // Firmware hash, written at provisioning only
    std::expected<std::vector<uint8_t>, ConfigError> firmware_hash() const;

private:
    DeviceConfig() = default;
    nvs_handle_t device_handle_;
    nvs_handle_t manifest_handle_;
    nvs_handle_t attestation_handle_;
};

} // namespace dev
