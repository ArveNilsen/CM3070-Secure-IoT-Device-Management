#pragma once

// Standard library includes
#include <functional>
#include <span>
#include <string_view>

// ESP-IDF includes
#include "mqtt_client.h"


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
    /**
     * @brief The result type of the class.
     * void or error
     */
    using result_type = std::expected<void, TransportError>;

    /**
     * @brief Constructor.
     * @param config A reference to the DeviceConfig object.
     */
    explicit NetworkTransport(DeviceConfig& config);

    /**
     * @brief Attempts to connect to the network with the given config
     * @retval TransportError
     * @return void
     */
    result_type connect();

    /**
     * @brief Attempts to disconnect from the network.
     * @return void
     */
    void disconnect();

    /**
     * @brief Check connected status.
     * @return bool
     */
    bool is_connected() const;

    result_type publish(std::string_view topic,
                std::span<const uint8_t> payload,
                int qos = 1);

    result_type subscribe(std::string_view topic,
                  MessageHandler handler);

private:
    DeviceConfig& config_;
    esp_mqtt_client_handle_t mqtt_client_ = nullptr;
};

} // namespace dev
