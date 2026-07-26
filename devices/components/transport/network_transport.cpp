#include "transport/network_transport.hpp"

using dev::NetworkTransport;
using result_type = dev::NetworkTransport::result_type;

NetworkTransport::NetworkTransport(DeviceConfig& config)
    : config_{config} {}

result_type NetworkTransport::connect()
{
    return {};
}

void NetworkTransport::disconnect()
{
    return;
}

bool NetworkTransport::is_connected() const
{
    return false;
}

result_type NetworkTransport::publish(std::string_view topic,
            std::span<const uint8_t> payload, int qos)
{
    return {};
}

result_type NetworkTransport::subscribe(std::string_view topic,
              MessageHandler handler)
{
    return {};
}
