#include "configs/device_config.hpp"

using dev::DeviceConfig;
using dev::ConfigError;
using result_type = dev::DeviceConfig::result_type;

DeviceConfig& DeviceConfig::instance()
{
    static DeviceConfig dc;
    return dc;
}

std::expected<void, ConfigError> DeviceConfig::init()
{
    return {};
}

result_type DeviceConfig::wifi_ssid() const
{
    return {};
}

result_type DeviceConfig::wifi_password() const
{
    return {};
}

result_type DeviceConfig::gateway_host() const
{
    return {};
}

result_type DeviceConfig::device_id() const
{
    return {};
}

result_type DeviceConfig::public_key_id() const
{
    return {};
}

std::string DeviceConfig::device_class() const
{
    return "";
}

bool DeviceConfig::is_enrolled() const
{
    return false;
}

std::expected<void, ConfigError> DeviceConfig::set_enrolled(bool enrolled)
{
    return {};
}

std::expected<std::vector<uint8_t>, ConfigError> 
DeviceConfig::manifest() const
{
    return {};
}

std::expected<void, ConfigError>
DeviceConfig::store_manifest(std::span<const uint8_t> manifest,
                   std::span<const uint8_t> signature)
{
    return {};
}

// Firmware hash, written at provisioning only
std::expected<std::vector<uint8_t>, ConfigError> 
DeviceConfig::firmware_hash() const
{
    return {};
}
