#pragma once

#include <functional>
#include <span>
#include <string_view>

namespace dev {

enum class TransportError {
    WifiFailure,
    BrokerUnreachable,
    PublishFailed,
    NotConnected
};

using MessageHandler = 
    std::function<void(std::string_view topic, 
                       std::span<const uint8_t> payload)>;

class NetworkTransport {
public:
    using result_type = std::expected<void, TransportError>;

    expicit NetworkTransport(DeviceConfig& config);

    result_type connect();
    void disconnect();
    bool is_connected const ();

    result_type publish(std::string_view topic,
                std::span<const uint8_t> payload,
                int qos = 1);

    result_type subscribe(std::string_view topic,
                  MessageHandler handler);

private:
    DeviceConfig& config_;
    esp_mqtt_client_handle_t mqtt_client_ = nullptr;

} // namespace dev
