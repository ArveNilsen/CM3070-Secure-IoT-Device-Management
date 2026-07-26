// Project includes
#include <wifi_station/esp32_wifi_driver.hpp>

// Std lib includes
#include <cstring>

// ESP-IDF includes
#include "esp_log.h"

namespace wifi_station::esp32 {

namespace {
constexpr const char* kTag = "wifi_station";
} // namespace anonymous

bool Esp32WifiDriver::netifInit()
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_netif_init failed: %d", err);
        return false;
    }
    
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_event_loop_create_default failed: %d", err);
        return false;
    }

    staNetif_ = esp_netif_create_default_wifi_sta();
    if (staNetif_ == nullptr) {
        ESP_LOGE(kTag, "esp_netif_create_default_wifi_sta failed: %d", err);
        return false;
    }

    return true;
}

bool Esp32WifiDriver::wifiInit()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_init failed: %d", err);
        return false;
    }

    return true;
}

bool Esp32WifiDriver::setStaConfig(const StaConfig& config)
{
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_set_mode failed: %d", err);
        return false;
    }

    wifi_config_t wifiConfig = {};
    // TODO: Remove calls to strncpy
    std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.ssid), 
        config.ssid.c_str(), sizeof(wifiConfig.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.password), 
        config.password.c_str(), sizeof(wifiConfig.sta.password) - 1);
    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    err = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_set_config failed: %d", err);
        return false;
    }

    return true;
}

bool Esp32WifiDriver::start()
{
    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_start_failed: %d", err);
        return false;
    }

    return true;
}

bool Esp32WifiDriver::connect()
{
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_connect failed: %d", err);
        return false;
    }

    return true;
}

bool Esp32WifiDriver::stop()
{
    esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_stop failed: %d", err);
        return false;
    }

    return true;
}

bool Esp32WifiDriver::deinit()
{
    esp_err_t err = esp_wifi_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_wifi_deinit failed: %d", err);
        return false;
    }

    return true;
}

namespace {

void onWifiEvent(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    auto& sm = *static_cast<WifiStationStateMachine*>(arg);

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
        sm.onWifiConnected();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        sm.onWifiDisconnected();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(data);
        char ipStr[16];
        esp_ip4addr_ntoa(&event->ip_info.ip, ipStr, sizeof(ipStr));
        sm.onGotIp(ipStr);
    }
}

} // namespace

esp_err_t registerEventGlue(WifiStationStateMachine& sm)
{
    esp_err_t err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
        &onWifiEvent, &sm);
    if (err != ESP_OK)
        return err;

    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
        &onWifiEvent, &sm);
    return err;
}

} // namespace wifi_station::esp32
