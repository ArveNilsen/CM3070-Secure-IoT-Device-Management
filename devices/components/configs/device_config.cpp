#include "configs/device_config.hpp"
#include "esp_log.h"
#include "esp_mac.h"
#include <cJSON.h>
#include <cstdio>
#include <format>

namespace dev {

namespace {

constexpr const char* TAG = "device_config";

constexpr const char* NS_DEVICE				= "device_cfg";
constexpr const char* NS_MANIFEST			= "manifest";
constexpr const char* NS_ATTESTATION	= "attestation";

namespace key {
// Read-only
constexpr const char* wifi_ssid = "wifi_ssid";
constexpr const char* wifi_password = "wifi_pass";
constexpr const char* gateway_host = "gw_host";
constexpr const char*	device_id = "device_id"; 
constexpr const char* public_key_id = "pubkey_id";

// Read-write
constexpr const char* enrolled = "enrolled";
constexpr const char* manifest = "manifest";
constexpr const char* signature = "signature";

constexpr const char* active_caps = "active_caps";

}	// namespace anon::key

} // namespace

// Meyer singleton
DeviceConfig& DeviceConfig::instance()
{
    static DeviceConfig singleton;
    return singleton;
}

// ---
// Lifecycle
// ---
std::expected<void, ConfigError> DeviceConfig::init()
{
		esp_err_t err = nvs_open_from_partition(
				"nvs_id", NS_DEVICE, NVS_READONLY, &device_handle_);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to open '%s' namespace: %s",
						NS_DEVICE, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		err = nvs_open_from_partition(
				"nvs_rt", NS_MANIFEST, NVS_READWRITE, &manifest_handle_);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to open '%s' namespace: %s",
						NS_DEVICE, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		// TODO: Currently unused, remove if no use.
		err = nvs_open_from_partition(
				"nvs_rt", NS_ATTESTATION, NVS_READWRITE, &attestation_handle_);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to open '%s' namespace: %s",
						NS_DEVICE, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		initialized_ = true;
    return {};
}

std::expected<void, ConfigError> DeviceConfig::ensure_initialized() const
{
	if (!initialized_) {
		ESP_LOGE(TAG, "Accessor called before init() succeded");
		return std::unexpected(ConfigError::NotInitialized);
	}

	return {};
}

// ---
// NVS helpers
// ---

namespace {

DeviceConfig::string_result get_string(nvs_handle_t handle, const char* key)
{
		size_t len = 0;	
		esp_err_t err = nvs_get_str(handle, key, nullptr, &len);
		if (err == ESP_ERR_NVS_NOT_FOUND) {
				ESP_LOGE(TAG, "Key: %s NOT FOUND", key);
				return std::unexpected(ConfigError::NotFound);
		}

		if (err != ESP_OK) {
				ESP_LOGE(TAG, "nvs_get_str size query failed for '%s': %s",
						key, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);	
		}

		std::string value(len, '\0');
		err = nvs_get_str(handle, key, value.data(), &len);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "nvs_get_str fetch failed for '%s': %s",
						key, esp_err_to_name(err));
			return std::unexpected(ConfigError::NVSFailure);
		}

		// NVS reported length includes null terminator.
		// Remove as it is unneeded in std::string
		if (!value.empty() && value.back() == '\0')
				value.pop_back();

		return value;
}

std::expected<void, ConfigError>
set_string(nvs_handle_t handle, const char* key, const std::string& value)
{
		esp_err_t err = nvs_set_str(handle, key, value.c_str());
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "nvs_set_str failed for '%s': %s",
						key, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		err = nvs_commit(handle);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "nvs_commit failed after setting '%s': %s",
						key, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		return {};
}

DeviceConfig::bytes_result
get_blob(nvs_handle_t handle, const char* key)
{
		size_t len = 0;
		esp_err_t err = nvs_get_blob(handle, key, nullptr, &len);
		if (err == ESP_ERR_NVS_NOT_FOUND)
				return std::unexpected(ConfigError::NotFound);

		if (err != ESP_OK) {
				ESP_LOGE(TAG, "nvs_get_blob size query failed for '%s': %s",
						key, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		std::vector<uint8_t> value(len);
		err = nvs_get_blob(handle, key, value.data(), &len);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "nvs_get_blob fetch failed for '%s': %s",
						key, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		return value;
}

std::expected<void, ConfigError>
set_blob(nvs_handle_t handle, const char* key, std::span<const uint8_t> value)
{
		esp_err_t err = nvs_set_blob(handle, key, value.data(), value.size());
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "nvs_set_blob failed for '%s': %s",
						key, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		err = nvs_commit(handle);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "nvs_commit failed after setting '%s': %s",
						key, esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		return {};
}

} // namespace

// ---
// Network
// ---

DeviceConfig::string_result DeviceConfig::wifi_ssid() const
{
		if (auto ok = ensure_initialized(); !ok)
			return std::unexpected(ok.error());

		return get_string(device_handle_, key::wifi_ssid);
}

DeviceConfig::string_result DeviceConfig::wifi_password() const
{
		if (auto ok = ensure_initialized(); !ok)
			return std::unexpected(ok.error());

		return get_string(device_handle_, key::wifi_password);
}

DeviceConfig::string_result DeviceConfig::gateway_host() const
{
		if (auto ok = ensure_initialized(); !ok)
			return std::unexpected(ok.error());

		return get_string(device_handle_, key::gateway_host);
}

DeviceConfig::string_result DeviceConfig::mac_addr() const
{
		std::array<uint8_t, 6> mac{};
		esp_err_t err = esp_read_mac(mac.data(), ESP_MAC_WIFI_STA);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to get mac adddress: %s",
						esp_err_to_name(err));
				return std::unexpected(ConfigError::HardwareFailure);
		}

		std::array<char, 18> buf{};
		int written = snprintf(buf.data(), buf.size(),
				"%02X:%02X:%02X:%02X:%02X:%02X",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		if (written < 0 || static_cast<size_t>(written) >= buf.size()) {
				ESP_LOGE(TAG, "MAC string formatting error");
				return std::unexpected(ConfigError::HardwareFailure);
		}

		return std::string{buf.data()};
}

// ---
// Identity
// ---

DeviceConfig::string_result DeviceConfig::device_id() const
{
		if (auto ok = ensure_initialized(); !ok)
			return std::unexpected(ok.error());

		return get_string(device_handle_, key::device_id);
}

DeviceConfig::string_result DeviceConfig::public_key_id() const
{
		if (auto ok = ensure_initialized(); !ok)
			return std::unexpected(ok.error());

		return get_string(device_handle_, key::public_key_id);
}

std::string DeviceConfig::device_class() const
{
		// Kconfig value
    return CONFIG_DEVICE_CLASS;
}

// ---
// Enrollment state
// ---

bool DeviceConfig::is_enrolled() const
{
		if (auto ok = ensure_initialized(); !ok)
				return false;

		uint8_t flag = 0;
		esp_err_t err = nvs_get_u8(manifest_handle_, key::enrolled, &flag);
		if (err != ESP_OK)
				return false; // key not found

		return flag != 0;
}

std::expected<void, ConfigError> DeviceConfig::set_enrolled(bool enrolled)
{
		if (auto ok = ensure_initialized(); !ok)
				return ok;

		esp_err_t err = nvs_set_u8(manifest_handle_, key::enrolled,
				enrolled ? 1 : 0);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to set enrolled flag: %s",
						esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		err = nvs_commit(manifest_handle_);
		if (err != ESP_OK)
				return std::unexpected(ConfigError::NVSFailure);

		return {};
}

// ---
// Manifest
// ---

DeviceConfig::bytes_result DeviceConfig::manifest() const
{
		if (auto ok = ensure_initialized(); !ok)
				return std::unexpected(ok.error());

		if (!is_enrolled())
				return std::unexpected(ConfigError::NotEnrolled);

		return get_blob(manifest_handle_, key::manifest);
}

std::expected<void, ConfigError>
DeviceConfig::store_manifest(std::span<const uint8_t> manifest,
														 std::span<const uint8_t> signature)
{
		if (auto ok = ensure_initialized(); !ok)
				return ok;

		// Both writes must succeed. Rollback if not.
		// TODO: Add limitation to report:
		// No multi-step transaction-commit support in nvs.
		if (auto ok = set_blob(manifest_handle_, key::manifest, manifest); !ok)
				return ok;

		if (auto ok = set_blob(manifest_handle_, key::signature, signature); !ok) {
				// Attempt rollback
				nvs_erase_key(manifest_handle_, key::manifest);
				nvs_commit(manifest_handle_);
				return ok;
		}

		return set_enrolled(true);
}

std::expected<uint32_t, ConfigError> DeviceConfig::capability_ceiling() const
{
		auto raw = manifest();
		if (!raw)
				return std::unexpected(raw.error());

		std::string manifest_str(raw->begin(), raw->end());
		cJSON* parsed = cJSON_ParseWithLength(
				manifest_str.data(), manifest_str.size());
		if (!parsed) {
				ESP_LOGE(TAG, "Stored manifest is not valid JSON");
				return std::unexpected(ConfigError::TypeMismatch);
		}

		cJSON* caps = cJSON_GetObjectItem(parsed, "capabilities");
		if (!cJSON_IsNumber(caps)) {
				ESP_LOGE(TAG, "Manifest missing numeric 'capabilities' field");
				cJSON_Delete(parsed);
				return std::unexpected(ConfigError::TypeMismatch);
		}

		uint32_t ceiling = static_cast<uint32_t>(caps->valuedouble);
		cJSON_Delete(parsed);
		return ceiling;
}

std::expected<uint32_t, ConfigError> DeviceConfig::active_capabilities() const
{
		if (auto ok = ensure_initialized(); !ok)
				return std::unexpected(ok.error());
		if (!is_enrolled())
				return std::unexpected(ConfigError::NotEnrolled);	

		uint32_t value = 0;
		esp_err_t err = nvs_get_u32(manifest_handle_, key::active_caps, &value);
		if (err == ESP_ERR_NVS_NOT_FOUND) {
				// Not yet written. CapabilityEnforcer treats this as full ceiling
				return std::unexpected(ConfigError::NotFound);
		}

		if (err != ESP_OK)
				return std::unexpected(ConfigError::NVSFailure);

		return value;
}

std::expected<void, ConfigError> 
DeviceConfig::store_active_capabilities(uint32_t mask)
{
		if (auto ok = ensure_initialized(); !ok)
				return ok;

		esp_err_t err = nvs_set_u32(manifest_handle_, key::active_caps, mask);
		if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to store active_caps: %s", esp_err_to_name(err));
				return std::unexpected(ConfigError::NVSFailure);
		}

		err = nvs_commit(manifest_handle_);
		if (err != ESP_OK)
				return std::unexpected(ConfigError::NVSFailure);

		return {};
}

} // namespace dev
