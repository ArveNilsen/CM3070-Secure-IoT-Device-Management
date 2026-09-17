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

/**
 * @brief Enumerates the bring-up steps
 */
enum class BringupStep { Ok, NetifInit, WifiInit, EventGlue, Configure, Start, Connect };

const char* toString(BringupStep step) noexcept;

BringupStep bringUpStation(WifiStationStateMachine& sm, const StaConfig& config);

class ConnectivityWaiter {
public:
	explicit ConnectivityWaiter(WifiStationStateMachine& sm) {
		events_ = xEventGroupCreate();
		sm.onPhaseChanged([this](Phase phase) {
			if (phase == Phase::GotIp) xEventGroupSetBits(events_, kGotIpBit);
			else											 xEventGroupClearBits(events_, kGotIpBit);
		});
	}

	~ConnectivityWaiter() { vEventGroupDelete(events_); }

	bool waitForGotIp(uint32_t timeoutMs) {
		EventBits_t bits = xEventGroupWaitBits(events_, kGotIpBit,
				pdFALSE, pdTRUE, pdMS_TO_TICKS(timeoutMs));
		return (bits & kGotIpBit) != 0;
	}

private:
	static constexpr EventBits_t kGotIpBit = BIT0;
	EventGroupHandle_t events_;
};
} // namespace wifi_station::esp32 
