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

// Architectural boundary note:
// Components use std::expected<T, E> throughout.
// main.cpp is the only file that converts to esp_err_t
// for use with ESP_ERROR_CHECK macros.
// Use dev::to_esp_err() for all such conversions.
// Never use .error_or() with ESP_ERROR_CHECK, the types
// are incompatible.

namespace {

const char *kTag = "APP_MAIN";

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

} // namespace dev

// Static/global lifetime objects:
wifi_station::esp32::Esp32WifiDriver a_driver;
wifi_station::WifiStationStateMachine a_sm(a_driver, /*maxRetries=*/5);

extern "C" void app_main()
{
    using namespace dev;

    // Wifi config is stored in NVS memory
    // Flash before use to avoid garbage data
    // Abort on error
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    // Initialise config and identity
    auto& config    = DeviceConfig::instance();
    ESP_ERROR_CHECK_WITHOUT_ABORT(to_esp_err(config.init()));

    ESP_ERROR_CHECK(err); // abort on error

		// Set up WiFi and connect to network
		wifi_station::esp32::ConnectivityWaiter waiter(a_sm);

		auto step = wifi_station::esp32::bringUpStation(a_sm, wifi_station::StaConfig{"Bjerkes residens", "3lvisinth3building"});
		if (step != wifi_station::esp32::BringupStep::Ok) {
			ESP_LOGE(kTag, "Wi-Fi bring-up failed at: %s", wifi_station::esp32::toString(step));
			return;
		}

		if (!waiter.waitForGotIp(20000)) {
			ESP_LOGE(kTag, "timed out waiting for GOT_IP");
			return;
		}

    ESP_LOGI(kTag, "Wi-Fi connected, ip=%s -- attempting gateway GET", a_sm.ip()->c_str());

    auto& identity  = HardwareIdentity::instance();
    ESP_ERROR_CHECK_WITHOUT_ABORT(to_esp_err(identity.init()));

    // Initialise transport
    NetworkTransport transport{config};
    transport.connect();

    // Enroll if not already enrolled
    if (!config.is_enrolled()) {
        EnrollmentService enrollment{config, identity};
        auto result = enrollment.enroll();

        if (!result) {
            ESP_LOGE(kTag, "Enrollment failed, halting");
            return;
        }

        config.store_manifest(result->manifest, result->gateway_signature);
    }

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
