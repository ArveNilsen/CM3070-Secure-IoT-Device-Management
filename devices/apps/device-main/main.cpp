#include "device_config.hpp"
#include "hardware_identity.hpp"
#include "enrollment.hpp"
#include "capability_enforcement.hpp"
#include "network_transport.hpp"
#include "command_handler.hpp"

extern "C" void app_main()
{
    using namespace dev;

    // Initialise config and identity
    auto& config    = DeviceConfig::instance();
    auto& identity  = HardwareIdentity::instance();

    ESP_ERROR_CHECK_WITHOUT_ABORT(config.init().error_or(ESP_OK));
    ESP_ERROR_CHECK_WITHOUT_ABORT(identity.init().error_or(ESP_OK));

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
