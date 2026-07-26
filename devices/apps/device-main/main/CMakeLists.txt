idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS ""
    REQUIRES
        # ESP-IDF
        "nvs_flash"
        mqtt
        # Project components
        configs
        identity
        enrollment
        capability
        command
        transport
)
