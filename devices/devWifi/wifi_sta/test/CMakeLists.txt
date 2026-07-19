enable_testing()

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/Cellar/catch2/3.7.1/lib/cmake")
find_package(Catch2 3 REQUIRED)

include(CTest)
include(Catch)

add_executable(tests)

target_sources(tests
    PRIVATE
        tests.cpp
        ../src/state_machine.cpp

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ../include
    FILES
        ../include/wifi_station/state_machine.hpp
        ../include/wifi_station/types.hpp
)

catch_discover_tests(tests)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

target_compile_options(tests PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -g
)

target_compile_features(tests PRIVATE cxx_std_17)

