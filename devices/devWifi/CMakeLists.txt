cmake_minimum_required(VERSION 4.0.2)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/wifi_sta")

project(wifidriver 
    LANGUAGES C
)
