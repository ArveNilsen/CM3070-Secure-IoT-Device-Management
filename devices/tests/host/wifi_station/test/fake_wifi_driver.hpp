#pragma once

#include <wifi_station/types.hpp>

namespace wifi_station::test {

/**
 * Test double for IWifiDriver.
 * Set appropriate field to false to test failure.
 * Inspect counters for deducing state transitions.
 */
class FakeWifiDriver : public IWifiDriver {
public:
    bool netifInitResult = true;
    bool wifiInitResult = true;
    bool setStaConfigResult = true;
    bool startResult = true;
    bool connectResult = true;
    bool stopResult = true;
    bool deinitResult = true;

    int netifInitCalls = 0;
    int wifiInitCalls = 0;
    int setStaConfigCalls = 0;
    int startCalls = 0;
    int connectCalls = 0;
    int stopCalls = 0;
    int deinitCalls = 0;

    StaConfig lastConfig;

    bool netifInit() override {
        ++netifInitCalls;
        return netifInitResult;
    }

    bool wifiInit() override {
        ++wifiInitCalls;
        return wifiInitResult;
    }

    bool setStaConfig(const StaConfig& config) override {
        ++setStaConfigCalls;
        lastConfig = config;
        return setStaConfigResult;
    }

    bool start() override {
        ++startCalls;
        return startResult;
    }

    bool connect() override {
        ++connectCalls;
        return connectResult;
    }

    bool stop() override {
        ++stopCalls;
        return stopResult;
    }

    bool deinit() override {
        ++deinitCalls;
        return deinitResult;
    }
};

}  // namespace wifi_station::test
