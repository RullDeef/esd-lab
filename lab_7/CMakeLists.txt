cmake_minimum_required(VERSION 3.10)

project(ExpertSystemDesignLabs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt on cache bool "" force)
FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB CORE_SOURCES src/core/*.cpp)
file(GLOB APP_SOURCES src/app/*.cpp)
file(GLOB TEST_SOURCES tests/*.cpp)

add_library(core ${CORE_SOURCES})
target_include_directories(core INTERFACE src/core)

add_executable(app ${APP_SOURCES})
target_link_libraries(app core)

add_executable(unittests ${TEST_SOURCES})
target_link_libraries(unittests core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unittests)
