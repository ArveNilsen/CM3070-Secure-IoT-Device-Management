#include <catch2/catch_test_macros.hpp>

#include <wifi_station/state_machine.hpp>
#include "fake_wifi_driver.hpp"

#include <iostream>

using wifi_station::Phase;
using wifi_station::StaConfig;
using wifi_station::WifiStationStateMachine;
using wifi_station::test::FakeWifiDriver;

namespace {
/**
 * From NotInit to Started, check every state.
 * Shared by several tests below.
 */
void bootToStarted(WifiStationStateMachine& sm) {
    CHECK(sm.netifInit());
    CHECK(sm.phase() == Phase::NetifInit);
    CHECK(sm.wifiInit());
    CHECK(sm.phase() == Phase::WifiInit);
    std::cout << "Phase before configure: " << wifi_station::toString(sm.phase()) << "\n";
    CHECK(sm.configure(StaConfig{"my-ssid", "my-password"}));
    CHECK(sm.phase() == Phase::Configured);
    CHECK(sm.configValid());
    CHECK(sm.start());
    CHECK(sm.phase() == Phase::Started);
}

}  // namespace

TEST_CASE("Test framework compiles and runs", "[basic]")
{
    REQUIRE(true);
}

TEST_CASE("Happy path reaches gotip", "[transition]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver);

    bootToStarted(sm);

    CHECK(sm.connect());
    CHECK(sm.phase() == Phase::Connecting);

    sm.onWifiConnected();
    CHECK(sm.phase() == Phase::Connected);
    CHECK(!sm.ip().has_value());

    sm.onGotIp("192.168.1.42");
    CHECK(sm.phase() == Phase::GotIp);
    CHECK(sm.ip().has_value());
    CHECK(*sm.ip() == "192.168.1.42");

    // Driver was actually asked to do each step exactly once.
    CHECK(driver.netifInitCalls == 1);
    CHECK(driver.wifiInitCalls == 1);
    CHECK(driver.setStaConfigCalls == 1);
    CHECK(driver.startCalls == 1);
    CHECK(driver.connectCalls == 1);
    CHECK(driver.lastConfig.ssid == "my-ssid");
}

TEST_CASE("Commands out of order are rejected without side effects", "[command]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver);

    // Nothing has happened yet: start() and connect() are not enabled.
    CHECK(!sm.start());
    CHECK(!sm.connect());
    CHECK(sm.phase() == Phase::NotInit);
    CHECK(driver.startCalls == 0);
    CHECK(driver.connectCalls == 0);

    CHECK(sm.netifInit());
    // Calling it again is not enabled a second time in a row.
    CHECK(!sm.netifInit());
    CHECK(sm.phase() == Phase::NetifInit);
}

TEST_CASE("Driver failure leaves phase unchanged", "[failure]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver);

    CHECK(sm.netifInit());
    driver.wifiInitResult = false;  // simulate esp_wifi_init() returning an error

    CHECK(!sm.wifiInit());
    CHECK(sm.phase() == Phase::NetifInit);  // unchanged: the command failed
    CHECK(driver.wifiInitCalls == 1);

    driver.wifiInitResult = true;  // recover, e.g. after a retry by the caller
    CHECK(sm.wifiInit());
    CHECK(sm.phase() == Phase::WifiInit);
}

TEST_CASE("Connect failure increments retry up to budget then is rejected", "[failure]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver, /*maxRetries=*/3);

    bootToStarted(sm);

    // First attempt is always allowed straight from Started.
    CHECK(sm.connect());
    sm.onWifiDisconnected();  // fails while Connecting -> ConnectFail
    CHECK(sm.phase() == Phase::Disconnected);
    CHECK(sm.retryCount() == 1);

    CHECK(sm.connect());
    sm.onWifiDisconnected();
    CHECK(sm.retryCount() == 2);

    CHECK(sm.connect());
    sm.onWifiDisconnected();
    CHECK(sm.retryCount() == 3);

    // Retry budget (3) is now exhausted: connect() must be rejected.
    CHECK(!sm.connect());
    CHECK(sm.phase() == Phase::Disconnected);
    CHECK(driver.connectCalls == 3);  // the 4th attempt never reached the driver
}

/**
 * Regression test to cover the reset retry bug found by TLC.
 * Retry count must be correctly reset.
 * Mirrors FreshSessionHasCleanBudget invariant.
 */
TEST_CASE("Stop resets retry budget for next session", "[regression]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver, /*maxRetries=*/3);

    bootToStarted(sm);
    for (int i = 0; i < 3; ++i) {
        CHECK(sm.connect());
        sm.onWifiDisconnected();
    }
    CHECK(sm.retryCount() == 3);

    CHECK(sm.stop());
    CHECK(sm.phase() == Phase::Stopped);
    CHECK(sm.retryCount() == 0);  // reset, not carried over

    CHECK(sm.configure(StaConfig{"my-ssid", "my-password"}));
    CHECK(sm.start());
    CHECK(sm.phase() == Phase::Started);

    // A fresh session: this must be allowed even though the previous
    // session had exhausted its budget.
    CHECK(sm.connect());
    CHECK(sm.phase() == Phase::Connecting);
}

TEST_CASE("Ip change updates address without leaving GotIp", "[transistion]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver);

    bootToStarted(sm);
    CHECK(sm.connect());
    sm.onWifiConnected();
    sm.onGotIp("10.0.0.5");
    CHECK(sm.phase() == Phase::GotIp);

    sm.onGotIp("10.0.0.9");  // e.g. DHCP lease renewed to a new address
    CHECK(sm.phase() == Phase::GotIp);
    CHECK(*sm.ip() == "10.0.0.9");
}

TEST_CASE("Unsolicited disconnect from GotIp drops ip and allows reconnect", "[transition]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver, /*maxRetries=*/3);

    bootToStarted(sm);
    CHECK(sm.connect());
    sm.onWifiConnected();
    sm.onGotIp("10.0.0.5");
    CHECK(sm.retryCount() == 0);

    sm.onWifiDisconnected();  // AP went out of range, say
    CHECK(sm.phase() == Phase::Disconnected);
    CHECK(!sm.ip().has_value());
    // An unsolicited disconnect from a fully-connected state should not
    // itself burn a retry attempt; only a failed CONNECTING does.
    CHECK(sm.retryCount() == 0);

    CHECK(sm.connect());
    CHECK(sm.phase() == Phase::Connecting);
}

TEST_CASE("Spurious events for the current phase are ignored", "[transitions]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver);

    // No connect ever issued: an unexpected "connected" event must not
    // move the phase.
    sm.onWifiConnected();
    CHECK(sm.phase() == Phase::NotInit);

    bootToStarted(sm);
    CHECK(sm.stop());
    CHECK(sm.phase() == Phase::Stopped);

    // Late-arriving event after Stop already fired: ignored.
    sm.onWifiDisconnected();
    CHECK(sm.phase() == Phase::Stopped);
}

TEST_CASE("Full lifecycle can restart after deinit", "[restart]") {
    FakeWifiDriver driver;
    WifiStationStateMachine sm(driver);

    bootToStarted(sm);
    CHECK(sm.stop());
    CHECK(sm.deinit());
    CHECK(sm.phase() == Phase::Deinit);
    CHECK(!sm.configValid());

    CHECK(sm.restart());
    CHECK(sm.phase() == Phase::NotInit);

    // Full cycle again from scratch.
    bootToStarted(sm);
    CHECK(sm.phase() == Phase::Started);
}
