// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF includes
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

// Project includes
#include "configs/device_config.hpp"
#include "identity/hardware_identity.hpp"
#include "enrollment/enrollment.hpp"
#include "capability/capability_enforcement.hpp"
#include "transport/network_transport.hpp"
#include "command/command_handler.hpp"
#include "wifi_station/esp32/esp32_wifi_driver.hpp"
#include "wifi_station/state_machine.hpp"

#include "cryptoauthlib.h"
#include "enrollment/hex_util.hpp"

// Architectural boundary note:
// Components use std::expected<T, E> throughout.
// main.cpp is the only file that converts to esp_err_t
// for use with ESP_ERROR_CHECK macros.
// Use dev::to_esp_err() for all such conversions.
// Never use .error_or() with ESP_ERROR_CHECK, the types
// are incompatible.

namespace {

const char *TAG = "APP_MAIN";

esp_err_t init_nvs_partition(const char* label)
{
		esp_err_t err = nvs_flash_init_partition(label);
		if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
				err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
				ESP_LOGW(TAG, "Erasing NVS partition '%s' "
											 "(corrupt or version mismatch)", label);
				ESP_ERROR_CHECK(nvs_flash_erase_partition(label));
				err = nvs_flash_init_partition(label);
		}

		return err;
}

} // namespace anon

namespace dev {

/**
 * @brief Convert std::expected<void, E> to esp_err_t
 * E must have a to_esp_err(E) overload visible
 */
template<typename E>
esp_err_t to_esp_err(std::expected<void, E> const& result) noexcept {
    if (result.has_value()) return ESP_OK;
    return to_esp_err(result.error());
}

/** 
 * @brief Convert std::expected<T, E> to esp_err_t, discarding value
 */
template<typename T, typename E>
esp_err_t to_esp_err(std::expected<T, E> const& result) noexcept {
    if (result.has_value()) return ESP_OK;
    return to_esp_err(result.error());
}

// Static/global lifetime objects:
wifi_station::esp32::Esp32WifiDriver a_driver;
wifi_station::WifiStationStateMachine a_sm(a_driver, /*maxRetries=*/5);

extern "C" void app_main()
{
		ESP_ERROR_CHECK(init_nvs_partition("nvs_id"));
		ESP_ERROR_CHECK(init_nvs_partition("nvs_rt"));

    auto& identity  = HardwareIdentity::instance();
    ESP_ERROR_CHECK_WITHOUT_ABORT(to_esp_err(identity.init()));

		uint8_t current_pubkey[64];
		ATCA_STATUS status = atcab_get_pubkey(CONFIG_ATTESTATION_KEY_SLOT, current_pubkey);
		if (status == ATCA_SUCCESS) {
				ESP_LOGI(TAG, "Current public key in slot: %s",
								 hex_encode(std::span(current_pubkey, 64)).c_str());
		} else {
				ESP_LOGE(TAG, "atcab_get_pubkey failed: 0x%02x", status);
		}

    // Initialise config and identity
    auto& config     = DeviceConfig::instance();
		auto init_result = config.init();
		if (!init_result) {
				ESP_LOGE(TAG, "DeviceConfig::init() failed. "
											 "Device is not provisioned - Halting.");
				return;
		}

		auto ssid = config.wifi_ssid();
		auto password = config.wifi_password();
		if (!ssid || !password) {
				ESP_LOGE(TAG, "Cannot bring up Wi-Fi -- SSID or password "
                  "not available from DeviceConfig. Halting.");
				return;
		}

		wifi_station::StaConfig sta_cfg{*ssid, *password};
		auto step = wifi_station::esp32::bringUpStation(a_sm, sta_cfg);

		// Set up WiFi and connect to network
		wifi_station::esp32::ConnectivityWaiter waiter(a_sm);

		if (step != wifi_station::esp32::BringupStep::Ok) {
			ESP_LOGE(TAG, "Wi-Fi bring-up failed at: %s", wifi_station::esp32::toString(step));
			return;
		}

		if (!waiter.waitForGotIp(20000)) {
			ESP_LOGE(TAG, "timed out waiting for GOT_IP");
			return;
		}

    ESP_LOGI(TAG, "Wi-Fi connected, ip=%s -- attempting gateway GET", a_sm.ip()->c_str());

    // Initialise transport
    NetworkTransport transport{config};
    transport.connect();

		ESP_LOGI(TAG, "Transport connected");

    // Enroll if not already enrolled
    if (!config.is_enrolled()) {
        EnrollmentService enrollment{config, identity};
				ESP_LOGI(TAG, "Starting enrollment...");
        if (auto result = enrollment.enroll(); !result) {
						switch (result.error()) {
						case EnrollmentError::AlreadyEnrolled:
								// Normal, log and recover
								ESP_LOGI(TAG, "Already enrolled, proceeding...");
								break;
						
						case EnrollmentError::StateMismatch:
								// Logged on error. Halt.
								ESP_LOGE(TAG, "Manual operator intervention needed.");
								return;

						default:
								ESP_LOGE(TAG, "Enrollment failed, halting");
								return;
						}

        } else {
						config.store_manifest(result->manifest, result->gateway_signature);
				}
    }

		ESP_LOGI(TAG, "Device already enrolled. Proceeding...");

    // Initialise capability enforcement from stored manifest
    CapabilityEnforcer  enforcer{config};
    CommandHandler      commands{enforcer, config};

    // Subscribe to gateway command topic
    transport.subscribe(
        "device/" + config.device_id().value_or("unknown") + "/commands",
        [&commands](auto topic, auto payload) {
            commands.handle(topic, payload);
        });

    // Main loop
    while (true) {
        if (auto ok = enforcer.check(Capability::PublishTelemetry); ok) {
            // Read and publish
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_HEARTBEAT_INTERVAL_MS));
    }
}
} // namespace dev
