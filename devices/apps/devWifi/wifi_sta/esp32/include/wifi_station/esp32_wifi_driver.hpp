#pragma once

// ESP includes
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

// Project includes
#include <wifi_station/state_machine.hpp>
#include <wifi_station/types.hpp>

namespace wifi_station::esp32 {

/**
 * @class Esp32WifiDriver
 * @ brief The Esp-Idf dependent implementation of IWifiDriver.
 *
 * Follows the spec (WiFiStation.tla)
 */
class Esp32WifiDriver : public IWifiDriver {
public:
    bool netifInit() override;
    bool wifiInit() override;
    bool setStaConfig(const StaConfig& config) override;
    bool start() override;
    bool connect() override;
    bool stop() override;
    bool deinit() override;

private:
    esp_netif_t* staNetif_ = nullptr;
};

/**
 * @brief Reisters event handlers on the event loop which are forwarded
 * into 'sm' event methods. Call once, after init, before start.
 * 'sm' must outlive the registered handlers.
 */
esp_err_t registerEventGlue(WifiStationStateMachine& sm);

} // namespace wifi_station::esp32 
