
/*
 * The Wi-Fi event handler.
 * Events implemented:
 *  WIFI_EVENT_STA_START
 *  WIFI_EVENT_STA_CONNECTED
 *  WIFI_EVENT_STA_DISCONNECTED 
 */
static void on_wifi_event(void *arg, esp_event_base_t event_base, 
        int32_t event_id, void *event_data)
{
    switch (event_id) {
    case WIFI_EVENT_STA_START:
        if (s_wifi_netif != NULL) {
            wifi_start(s_wifi_netif, event_base, event_id, event_data);
        }
        break;

    case WIFI_EVENT_STA_CONNECTED:
        // Handle connection, register callback, start DHCP
        wifi_event_sta_connected_t *event_sta_connected = 
            (wifi_event_sta_connected_t *)event_data;
        ESP_LOGI(TAG, "Connected to AP: %s",
                event_sta_connected->ssid);

        // Register interface receive callback
        wifi_netif_driver_t driver = 
            esp_netif_get_io_driver(s_wifi_netif);
        esp_wifi_register_if_rxcb(driver, esp_netif_receive, s_wifi_netif);

        // Start DHCP process
        esp_netif_action_connected(s_wifi_netif, event_base, event_id, 
                event_data);
        xEventGroupSetBits(s_wifi_event_group, WIFI_STA_CONNECTED_BIT);
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        esp_netif_action_disconnected(s_wifi_netif, event_base,
                event_id, event_data);
        xEventGroupClearBits(s_wifi_event_group, WIFI_STA_CONNECTED_BIT);
        break;
    }
}

/*
 * The IP event handler.
 * Events implemented:
 *  IP_EVENT_STA_GOT_IP 
 *  IP_EVENT_STA_LOST_IP
 */
static void on_ip_event(void *arg, esp_event_base_t event_base, 
        int32_t event_id, void *event_data)
{
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
        esp_wifi_internal_set_sta_ip();
        xEventGroupSetBits(s_wifi_event_group,
                WIFI_STA_IPV4_OBTAINED_BIT);

        ip_event_got_ip_t *event_ip =
            (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP address: " IPSTR, IP2STR(&event_ip->ip_info.ip));
        break;

    case IP_EVENT_STA_LOST_IP:
        xEventGroupClearBits(s_wifi_event_group, WIFI_STA_IPV4_OBTAINED_BIT);
        break;
    }
}

esp_err_t wifi_sta_init(EventGroupHandle_t event_group)
{
    esp_err_t esp_ret;

    // Save the current event group handle
    if (event_group != NULL)
        s_wifi_event_group = event_group;

    // Create default wifi network interface
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_WIFI_STA();
    s_wifi_netif = esp_netif_new(&netif_cfg);

    // Create wifi driver and attach to network interface
    s_wifi_driver = esp_wifi_create_if_driver(WIFI_IF_STA);
    esp_ret = esp_netif_attach(s_wifi_netif, s_wifi_driver);

    // Register wifi events
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START,
            &on_wifi_event, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
            &on_wifi_event, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
            &on_wifi_event, NULL);

    // Register IP events
#if CONFIG_WIFI_STA_CONNECT_IPV4 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
            &on_ip_event, NULL);
#endif

    // Initialise wifi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wsp_wifi_init(&cfg);

    // Set mode to station
    esp_wifi_set_mode(WIFI_MODE_STA);

    // Configure connection with Kconfig values
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_STA_SSID,
            .password = CONFIG_WIFI_STA_PASSWORD,
            .threshold.authmode = auth_mode,
            .sae_pwe_h2e = sae_pwe_method,
        },
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    // Start wifi, trigger WIFI_EVENT_START
    esp_wifi_start();

    return ESP_OK;

}
