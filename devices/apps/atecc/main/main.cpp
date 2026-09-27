#include <stdio.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Microchip CryptoAuthLib headers
#include "cryptoauthlib.h"

static const char *TAG = "atecc";

static EventGroupHandle_t s_wifi_event_group;
static constexpr int WIFI_CONNECTED_BIT = BIT0;

#define WIFI_SSID     "ssid"
#define WIFI_PASSWORD "pwd"

static void wifi_event_handler(void* arg, esp_event_base_t base,
                               int32_t id, void* data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_connect_blocking()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    s_wifi_event_group = xEventGroupCreate();
    esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr);
    esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, nullptr);

    wifi_config_t sta_cfg = {};
    strncpy((char*)sta_cfg.sta.ssid, WIFI_SSID,
            sizeof(sta_cfg.sta.ssid));
    strncpy((char*)sta_cfg.sta.password, WIFI_PASSWORD,
            sizeof(sta_cfg.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi connected.");
}

static bool atecc_probe(const char* label);

static ATCAIfaceCfg cfg = {};

static bool atecc_init()
{
    cfg.atcai2c.address = 0xC0;
    cfg.iface_type = ATCA_I2C_IFACE;
    cfg.devtype = ATECC608B;
    cfg.atcai2c.bus = 0;
    cfg.atcai2c.baud = 100000;
    cfg.wake_delay = 1500;
    cfg.rx_retries = 5;

    ATCA_STATUS status = atcab_init(&cfg);
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "atcab_init failed: 0x%02x", status);
        return false;
    }

    return true;
}

static bool atecc_probe(const char* label)
{
		ATCA_STATUS wake_status = atcab_wakeup();
    if (wake_status != ATCA_SUCCESS) {
        ESP_LOGW(TAG, "[%s] atcab_wakeup failed: 0x%02x",
                 label, wake_status);

    }
    uint8_t revision[4];
    ATCA_STATUS status = atcab_info(revision);
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "[%s] atcab_info FAILED: 0x%02x", label, status);
        return false;
    }
    ESP_LOGI(TAG, "[%s] atcab_info OK: %02x %02x %02x %02x",
             label, revision[0], revision[1], revision[2], revision[3]);
    return true;
}

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());

    if (!atecc_init()) return;

    // Baseline: confirm the chip works before ANYTHING else.
    atecc_probe("baseline, pre-wifi");

    // --- TEST BLOCK 1: does mere Wi-Fi CONNECTION (no data
    // transfer at all) break it, or only active TX? ---
    //wifi_connect_blocking();
    atecc_probe("immediately post-connect, no data sent");

    // --- TEST BLOCK 2: repeated probes over time, to see
    // if failure is immediate-after-connect or appears only
    // after some delay / after repeated TX activity ---
    for (int i = 0; i < 10; ++i) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        char label[32];
        snprintf(label, sizeof(label), "post-connect +%ds", i + 1);
        atecc_probe(label);
    }

    // --- TEST BLOCK 3: force active TX (a UDP broadcast,
    // no TCP/HTTP/TLS involved) immediately before a probe,
    // to isolate "TX power draw" from "HTTP/TCP stack
    // complexity" as the trigger ---
    //
    // (left as a stub -- add a raw UDP send here if blocks
    // 1/2 don't already reproduce the failure, to narrow
    // further between "any Wi-Fi activity" and "specifically
    // heavier TCP/HTTP traffic")

    ESP_LOGI(TAG, "Test sequence complete.");
}

