add_library(wifi_station_core)

target_include_directories(wifi_station_core PUBLIC include)

target_sources(wifi_station_core
    PRIVATE
        state_machine.cpp

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ../include
    FILES
        ../include/wifi_station/state_machine.hpp
        ../include/wifi_station/types.hpp
)

