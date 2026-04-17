cmake_minimum_required(VERSION 3.22)
project(ticket_system CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Zależności przez Conan
find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

# === Serwer ===
add_executable(server src/server/main.cpp)
target_link_libraries(server PRIVATE httplib::httplib nlohmann_json::nlohmann_json)

# === Klient ===
add_executable(client src/client/main.cpp)
target_link_libraries(client PRIVATE httplib::httplib nlohmann_json::nlohmann_json)

# === Testy ===
enable_testing()
add_executable(tests tests/main_test.cpp)
target_link_libraries(tests PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)