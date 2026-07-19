#include "wifi_station/statemachine.hpp"

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

} // namespace wifi_station
