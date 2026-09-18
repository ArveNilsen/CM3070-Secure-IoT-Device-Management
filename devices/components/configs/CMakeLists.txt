idf_component_register(
    SRCS "device_config.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
		PRIV_REQUIRES json
)
