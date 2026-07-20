// ESP-IDF includes
#include "esp_log.h"
#include "nvs_flash.h"

// Project includes
#include <wifi_station/esp32_wifi_driver.hpp>
#include <wifi_station/state_machine.hpp>

using wifi_station::Phase; // for test
using wifi_station::StaConfig;
using wifi_station::WifiStationStateMachine;
using wifi_station::esp32::Esp32WifiDriver;
using wifi_station::esp32::registerEventGlue;

namespace {

constexpr const char* kTag = "app_main";

// Static/global lifetime objects:
Esp32WifiDriver a_driver;
WifiStationStateMachine a_sm(a_driver, /*maxRetries=*/5);

} // namespace

extern "C" void app_main()
{
    // Wifi config is stored in NVS memory
    // Flash before use to avoid garbage data
    // Abort on error
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err); // abort on error

    // Init netif and wifi
    if (!a_sm.netifInit()) {
        ESP_LOGE(kTag, "netifInit failed");
        return;
    }
    if (!a_sm.wifiInit()) {
        ESP_LOGE(kTag, "wifiInit failed");
        return;
    }

    // Register eventhandler, abort on error
    ESP_ERROR_CHECK(registerEventGlue(a_sm));

    // Configure ssid and password
    // TODO: Get from esp-idf config instead of hardcoded
    if (!a_sm.configure(StaConfig{"ssid", "pwd"})) {
        ESP_LOGE(kTag, "configure failed");
        return;
    }
    //
    // Start wifi, get ready to connect
    if (!a_sm.start()) {
        ESP_LOGE(kTag, "start failed");
        return;
    }

    // Finally, connect
    if (!a_sm.connect()) {
        ESP_LOGE(kTag, "connect failed");
        return;
    }

    // --- The WiFi connectionis event-driven from here ---

    // Poll for GOT_IP. Only for simple test
    // integration should react to onGotIp instead.
    constexpr int kMaxWaitSeconds = 20;
    int waited = 0;
    while (a_sm.phase() != Phase::GotIp && waited < kMaxWaitSeconds) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ++waited;
    }

    if (a_sm.phase() != Phase::GotIp) {
        ESP_LOGE(kTag, "timed out waiting for GOT_IP (phase=%s)",
                 wifi_station::toString(a_sm.phase()));
        return;
    }

    ESP_LOGI(kTag, "Wi-Fi connected, ip=%s -- attempting gateway GET", a_sm.ip()->c_str());
}
