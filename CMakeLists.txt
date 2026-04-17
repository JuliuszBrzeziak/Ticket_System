cmake_minimum_required(VERSION 3.22)
project(ticket_system CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

add_library(core
    src/core/ChangeCalculator.cpp
    src/core/TicketPool.cpp
)

target_include_directories(core PUBLIC src)

add_executable(server src/server/main.cpp)
target_link_libraries(server PRIVATE core httplib::httplib nlohmann_json::nlohmann_json)

add_executable(client src/client/main.cpp)
target_link_libraries(client PRIVATE core httplib::httplib nlohmann_json::nlohmann_json)

enable_testing()

add_executable(tests
    tests/main_test.cpp
    tests/change_calculator_test.cpp
)

target_link_libraries(tests PRIVATE core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)