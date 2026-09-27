cmake_minimum_required(VERSION 3.30.2)

project(host_tests
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)

find_package(Catch2 3 REQUIRED)

include(CTest)
include(Catch)

add_executable(test_wifi_station
	${CMAKE_SOURCE_DIR}/../../components/wifi_station/src/state_machine.cpp
	wifi_station/test_wifi_station_state_machine.cpp
)

target_include_directories(test_wifi_station
	PRIVATE
		${CMAKE_SOURCE_DIR}/../../components/wifi_station/include
		wifi_station
)

catch_discover_tests(test_wifi_station)

target_link_libraries(test_wifi_station
    PRIVATE
        Catch2::Catch2WithMain
)

enable_testing()
