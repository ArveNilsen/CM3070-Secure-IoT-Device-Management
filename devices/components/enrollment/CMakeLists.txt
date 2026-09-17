list(APPEND sdkconfig_defaults $ENV{IDF_PATH}/components/mbedtls/config/mbedtls_preset_default.conf)

idf_component_register(
    SRCS "enrollment.cpp"
    INCLUDE_DIRS "include"
		PRIV_REQUIRES esp_http_client mbedtls esp_timer
		REQUIRES configs identity
)
