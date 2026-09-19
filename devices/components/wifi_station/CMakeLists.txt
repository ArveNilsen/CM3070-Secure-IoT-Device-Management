idf_component_register(
	SRCS
		"src/state_machine.cpp"
		"src/esp32/esp32_wifi_driver.cpp"
	INCLUDE_DIRS "include"
	PRIV_REQUIRES esp_event
	REQUIRES esp_netif esp_wifi 
)
