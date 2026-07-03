cmake_minimum_required(VERSION 3.20)
project(astromech_control_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ace
    src/actuator_bus.cpp
    src/actuator_serial.cpp
    src/actuator_virtual.cpp
    src/json.cpp
    src/scheduler.cpp
    src/sequence.cpp
    src/sync_engine.cpp
    src/telemetry.cpp
    src/trigger.cpp
)

target_include_directories(ace PUBLIC include)

add_executable(ace_cli apps/ace_cli/main.cpp)
target_link_libraries(ace_cli PRIVATE ace)

add_executable(ace_tests tests/ace_tests.cpp)
target_link_libraries(ace_tests PRIVATE ace)

add_executable(stress_actuators bench/stress_actuators.cpp)
target_link_libraries(stress_actuators PRIVATE ace)

enable_testing()
add_test(NAME ace_tests COMMAND ace_tests)
