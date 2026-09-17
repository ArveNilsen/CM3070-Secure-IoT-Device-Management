#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <functional>
#include <utility>

#include <wifi_station/types.hpp>

namespace wifi_station {

/**
 * Currently no effort to be made thread safe. Call from single task.
 */
class WifiStationStateMachine {
public:
    explicit WifiStationStateMachine(IWifiDriver& driver, std::uint32_t maxRetries = 3)
        : driver_(driver), maxRetries_(maxRetries) {}

    /**
     * Phase 1a. Precondition: phase == NotInit.
     */
    bool netifInit();

    /**
     * Phase 1b. Precondition: phase == NetifInit.
     */
    bool wifiInit();

    /** 
     * Phase 2. Precondition: phase in {WifiInit, Stopped}.
     */
    bool configure(const StaConfig& config);

    /** 
     * Phase 3. Precondition: phase == Configure implies config is valid
     */
    bool start();

    /** 
     * Phase 4. Precondition: phase == Started || 
     * phase == Disconnected with retryCount < maxRetries
     */
    bool connect();

    /** 
     * Phase 8a. Precondition: phase in {Started, Connecting, Connected, GotIP,
     * Disconnected}. Resets the retry budget.
     */
    bool stop();

    /** 
     * Phase 8b. Precondition: phase == Stopped
     */
    bool deinit();

    /** 
     * Helper phase for testing.
     * Simulates device restart, fresh from boot.
     */
    bool restart();

    // --- Events ---

    /**
     * Event handler on connect.
     * Ignored outside of Connecting state
     */
    void onWifiConnected();

    /**
     * Event handler for disconnect
     * Failed connect or drop. See spec.
     */
    void onWifiDisconnected();

    /**
     * Connected => GotIp, else IPChanged
     */
    void onGotIp(const std::string& ip);

		using PhaseChangeCallback = std::function<void(Phase)>;

		/**
		 *
		 */
		void onPhaseChanged(PhaseChangeCallback callback) {
			onPhaseChanged_ = std::move(callback);
		}

    // --- Introspection ---
    Phase phase() const noexcept { return phase_; }
    std::uint32_t retryCount() const noexcept { return retryCount_; }
    bool configValid() const noexcept { return configValid_; }
    const std::optional<std::string>& ip() const noexcept { return ip_; }

private:
    IWifiDriver& driver_;
    std::uint32_t maxRetries_;
    Phase phase_ = Phase::NotInit;
    std::uint32_t retryCount_ = 0;
    bool configValid_ = false;
    std::optional<std::string> ip_;

		PhaseChangeCallback onPhaseChanged_;

		/**
		 * Single point of phase transitions
		 */
		void setPhase(Phase newPhase) {
			phase_ = newPhase;
			if (onPhaseChanged_) 
				onPhaseChanged_(phase_);
		}
};
} // namespace wifi_station
