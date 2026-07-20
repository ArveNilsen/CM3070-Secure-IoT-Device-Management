cmake_minimum_required(VERSION 4.0.2)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

project(wifi_station 
    VERSION 0.0.1
    LANGUAGES CXX
)

include(FetchContent)
include(cmake/ProjectSettings.cmake)
include(cmake/ProjectOptions.cmake)
include(cmake/Doxygen.cmake)

project_local_options()
project_enable_doxygen("")

add_subdirectory(src)
add_subdirectory(test)
