/*
 * SPDX-FileCopyrightText: 2025 Shawn Hymel
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This module is based on Shawn Hymel's guide found here:
 * https://shawnhymel.com/2967/esp32-creating-a-wifi-driver-with-esp-idf/
 *
 * Writing Wi-Fi documentation from EspressIf:
 * https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/api-guides/wifi.html
 *
 * Wi-Fi API reference from EspressIf:
 * https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/api-reference/network/esp_wifi.html
 *
 * It will end up as an adaption as I make changes where needed. 
 * I will mark the sections that are my own code.
 */

#ifndef AN_WIFI_STA_H
#define AN_WIFI_STA_H

#include "esp_err.h"

#define WIFI_STA_CONNECTED_BIT      BIT0
#define WIFI_STA_IPV4_OBTAINED_BIT  BIT1
#define WIFI_STA_IPV6_OBTAINED_BIT  BIT2

/*
 * Initialise Wi-Fi in station (STA) mode.
 *
 * Use param for wait and IP assignment.
 *
 * Pre-condition: esp_netif_init() and esp_event_loop_create_loop_default
 *  must be called before this function.
 */
esp_err_t wifi_sta_init(EventGroupHandle_t event_group);

/*
 * Disable Wi-Fi
 */
esp_err_t wifi_sta_stop(void);

/*
 * Attempt to reconnect
 */
esp_err_t wifi_sta_reconnect(void);

#endif // AN_WIFI_STA_H
