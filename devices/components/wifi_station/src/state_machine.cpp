#include <wifi_station/state_machine.hpp>

#include <cassert>

namespace wifi_station {

const char* toString(Phase phase) noexcept {
    switch (phase) {
        case Phase::NotInit:        return "NOT_INIT";
        case Phase::NetifInit:      return "NETIF_INIT";
        case Phase::WifiInit:       return "WIFI_INIT";
        case Phase::Configured:     return "CONFIGURED";
        case Phase::Started:        return "STARTED";
        case Phase::Connecting:     return "CONNECTING";
        case Phase::Connected:      return "CONNECTED";
        case Phase::GotIp:          return "GOT_IP";
        case Phase::Disconnected:   return "DISCONNECTED";
        case Phase::Stopped:        return "STOPPED";
        case Phase::Deinit:         return "DEINIT";
        default: assert(false && "Phase::toString unhandled case");
    }
    return "UNKNOWN";
}

// --- Wifi commands ---

bool WifiStationStateMachine::netifInit()
{
    if (phase_ != Phase::NotInit) 
        return false;

    if (!driver_.netifInit())
        return false;

    setPhase(Phase::NetifInit);
    return true;
}

bool WifiStationStateMachine::wifiInit()
{
    if (phase_  != Phase::NetifInit)
        return false;

    if (!driver_.wifiInit())
        return false;

    setPhase(Phase::WifiInit);
    return true;
}

bool WifiStationStateMachine::configure(const StaConfig& config)
{
    if (phase_ != Phase::WifiInit && phase_ != Phase::Stopped)
        return false;

    if (!driver_.setStaConfig(config))
        return false;

    configValid_ = true;
    setPhase(Phase::Configured);
    return true;
}

bool WifiStationStateMachine::start()
{
    if (phase_ != Phase::Configured || !configValid_)
        return false;

    if (!driver_.start())
        return false;

    setPhase(Phase::Started);
    return true;
}

bool WifiStationStateMachine::connect()
{
    const bool preconditionsHolds =
        phase_ == Phase::Started ||
        (phase_ == Phase::Disconnected && retryCount_ < maxRetries_);
    
    if (!preconditionsHolds)
        return false;

    if (!driver_.connect())
        return false;

    setPhase(Phase::Connecting);
    return true;
}

bool WifiStationStateMachine::stop()
{
    switch (phase_) {
        case Phase::Started:
        case Phase::Connecting:
        case Phase::Connected:
        case Phase::GotIp:
        case Phase::Disconnected:
            break;
        default:
            return false;
    }

    if (!driver_.stop())
        return false;

    ip_.reset();
    retryCount_ = 0;
    setPhase(Phase::Stopped);
    return true;
}

bool WifiStationStateMachine::deinit()
{
    if (phase_ != Phase::Stopped)
        return false;

    if (!driver_.deinit())
        return false;

    retryCount_ = 0;
    configValid_ = false;
    setPhase(Phase::Deinit);
    return true;
}

bool WifiStationStateMachine::restart()
{
    if (phase_ != Phase::Deinit)
        return false;

    setPhase(Phase::NotInit);
    return true;
}


// --- Events ---

void WifiStationStateMachine::onWifiConnected()
{
    if (phase_ != Phase::Connecting)
        return; // ignore Connecting state

    retryCount_ = 0;
    setPhase(Phase::Connected);
}

void WifiStationStateMachine::onWifiDisconnected()
{
    switch (phase_) {
        case Phase::Connecting:
            ++retryCount_;
            setPhase(Phase::Disconnected);
            break;
        case Phase::Connected:
        case Phase::GotIp:
            ip_.reset();
            setPhase(Phase::Disconnected);
            break;
        default:
            break; // Ignore all other states
    }
}

void WifiStationStateMachine::onGotIp(const std::string& newIp)
{
    if (phase_ == Phase::Connected) {
        ip_ = newIp;
        setPhase(Phase::GotIp);
    } else if (phase_ == Phase::GotIp && ip_ != newIp) {
        ip_ = newIp; // IPChange
    }
    
    // Ignore all other states
}

} // namespace wifi_station
