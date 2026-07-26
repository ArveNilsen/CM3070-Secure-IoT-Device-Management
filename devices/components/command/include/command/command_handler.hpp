#pragma once

#include <span>
#include <string_view>

namespace dev {

class CapabilityEnforcer;
class DeviceConfig;

class CommandHandler {
public:
    explicit CommandHandler(
        CapabilityEnforcer& enforcer,
        DeviceConfig&       config);

    /**
     * @brief Called by network_transport on message receipt
     */
    void handle(std::string_view topic, std::span<const uint8_t> payload);

private:
    void handle_restrict(std::span<const uint8_t> payload);
    void handle_quarantine();
    void handle_restore(std::span<const uint8_t> payload);
    void handle_revoke();

    CapabilityEnforcer& enforcer_;
    DeviceConfig&       config_;
};

} // namespace dev
