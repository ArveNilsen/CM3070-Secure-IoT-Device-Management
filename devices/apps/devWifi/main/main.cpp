// ESP-IDF includes
#include "esp_log.h"
#include "nvs_flash.h"

// Project includes
#include <wifi_station/esp32_wifi_driver.hpp>
#include <wifi_station/state_machine.hpp>

using wifi_station::Phase;
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

		wifi_station::esp32::ConnectivityWaiter waiter(a_sm);

		auto step = wifi_station::esp32::bringUpStation(a_sm, StaConfig{"ssid", "pwd"});
		if (step != wifi_station::esp32::BringupStep::Ok) {
			ESP_LOGE(kTag, "Wi-Fi bring-up failed at: %s", wifi_station::esp32::toString(step));
			return;
		}

		if (!waiter.waitForGotIp(20000)) {
			ESP_LOGE(kTag, "timed out waiting for GOT_IP");
			return;
		}

    ESP_LOGI(kTag, "Wi-Fi connected, ip=%s -- attempting gateway GET", a_sm.ip()->c_str());
}
