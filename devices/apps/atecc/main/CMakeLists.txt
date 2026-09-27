idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
											 PRIV_REQUIRES 
													esp-cryptoauthlib
													esp_wifi
													esp_netif
													nvs_flash
													esp_event
)

