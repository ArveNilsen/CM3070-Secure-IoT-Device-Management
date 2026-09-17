#pragma once

#include <string>

namespace wifi_station {

/**
 * This mirrors the phases set in the TLA+ spec (WiFiStation.tla)
 */
enum class Phase {
    NotInit,
    NetifInit,
    WifiInit,
    Configured,
    Started,
    Connecting,
    Connected,
    GotIp,
    Disconnected,
    Stopped,
    Deinit
};

/**
 * Data for STA configuration
 */
struct StaConfig {
    std::string ssid;
    std::string password;
};

const char* toString(Phase phase) noexcept;

/**
 * The WiFi Driver interface
 */
class IWifiDriver {
public:
    virtual ~IWifiDriver() = default;

    // Phase 1: esp_netif_init() + esp_event_loop_create()
    virtual bool netifInit() = 0;

    // Phase 1: esp_wifi_init(&cfg)
    virtual bool wifiInit() = 0;

    // Phase 2: esp_wifi_set_mode(WIFI_MODE_STA) + esp_wifi_set_config(...)
    virtual bool setStaConfig(const StaConfig& config) = 0;

    // Phase 3: esp_wifi_start()
    virtual bool start() = 0;

    // Phase 4: esp_wifi_connect()
    virtual bool connect() = 0;

    // Phase 8: esp_wifi_stop()
    virtual bool stop() = 0;

    // Phase 8: esp_wifi_deinit()
    virtual bool deinit() = 0;
};

} // namespace wifi_station
