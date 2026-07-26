// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF includes
#include "esp_log.h"
#include "esp_err.h"

// Project includes
#include "configs/device_config.hpp"
#include "identity/hardware_identity.hpp"
#include "enrollment/enrollment.hpp"
#include "capability/capability_enforcement.hpp"
#include "transport/network_transport.hpp"
#include "command/command_handler.hpp"

// TODO: Add to config and remove here
#define CONFIG_HEARTBEAT_INTERVAL_MS 100

// Architectural boundary note:
// Components use std::expected<T, E> throughout.
// main.cpp is the only file that converts to esp_err_t
// for use with ESP_ERROR_CHECK macros.
// Use dev::to_esp_err() for all such conversions.
// Never use .error_or() with ESP_ERROR_CHECK, the types
// are incompatible.

namespace {

const char *TAG = "APP_MAIN";

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

extern "C" void app_main()
{
    using namespace dev;

    // Initialise config and identity
    auto& config    = DeviceConfig::instance();
    ESP_ERROR_CHECK_WITHOUT_ABORT(to_esp_err(config.init()));

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
            ESP_LOGE(TAG, "Enrollment failed, halting");
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
