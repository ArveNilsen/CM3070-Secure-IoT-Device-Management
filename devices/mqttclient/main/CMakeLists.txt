idf_component_register(SRCS "mqttclient_main.c"
                       PRIV_REQUIRES mqtt nvs_flash esp_netif
                       INCLUDE_DIRS "")
