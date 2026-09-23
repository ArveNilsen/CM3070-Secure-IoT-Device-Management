#pragma once

/*
 * Adapted and translated to C++ from the official EspressIf C examples:
 * https://github.com/espressif/esp-idf/blob/v6.0.2/examples/protocols/esp_http_client/main/esp_http_client_example.c
 */

static const char* hTag = "NonceClient";

class NonceClient {
public:
    NonceClient() {}

    sendGetNonce() {}

private:
    std::string path_to_nonce_server_;  // TODO: Consider setting at compile time.
    std::string response_buffer_;       // TODO: Consider static storage backed by bump allocator

    // Static routing callback wrapper for C-api
    static esp_err_t httpEventHandler(esp_http_client_event_t *evt)
    {
        NonceClient* nc = static_cast<NonceClient*>(evt->user_data);
        if (nc != nullptr)
            return nc->handleEvent(evt);

        return ESP_OK;
    }

    esp_err_t handleEvent(esp_client_event_t *evt)
    {
        // esp_http_client_event_id_t enum:
        // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_client.html#enumerations
        switch (evt->event_id) {
            case HTTP_EVENT_ERROR:
                ESP_LOGD(hTag, "HTTP_EVENT_ERROR");
                break;
            case HTTP_EVENT_ON_CONNECTED:
                ESP_LOGD(hTag, "HTTP_EVENT_ON_CONNECTED");
                break;
            case HTTP_EVENT_HEADERS_SENT:
                ESP_LOGD(hTag, "HTTP_EVENT_HEADER_SENT");
                break;
            case HTTP_EVENT_ON_HEADER:
                ESP_LOGD(hTag, "HTTP_EVENT_ON_HEADER: %s = %s", evt->header_key, evt->header_value);
                break;
            case HTTP_EVENT_ON_HEADERS_COMPLETE:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
                break;
            case HTTP_EVENT_ON_DATA:
                ESP_LOGD(hTag, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                /*
                 * 1. Clear buffer.
                 * 2. Check if response is chunked.
                 * 3a. Handle if chunked.
                 * 3b. Handle if not chunked.
                 * 4. ...
                 */
                break;
            case HTTP_EVENT_ON_FINISH:
                ESP_LOGD(hTag, "HTTP_EVENT_ON_FINISH");
                /*
                 * What now?
                 * Dump buffer?
                 */
                break;
            case HTTP_EVENT_DISCONNECTED:
                ESP_LOGD(hTag, "HTTP_EVENT_DISCONNECTED");
                /*
                 * TLS or not is relevant here.
                 * Decide what to do with disconnects.
                 */
                break;
            case HTTP_EVENT_REDIRECT:
                ESP_LOGD(hTag, "HTTP_EVENT_REDIRECT");
                /*
                 * Redirects should never happen in this system.
                 */
                break;
        }

        return ESP_OK;
    }
};
