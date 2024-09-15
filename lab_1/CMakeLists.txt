cmake_minimum_required(VERSION 3.10)

project(ExpertSystemDesignLabs)

set(CMAKE_CXX_STANDARD 17)
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

add_library(core 
    src/dict.cpp
    src/graph_search.cpp
    src/graph_viz.cpp
    src/rule_printer.cpp
)
target_include_directories(core INTERFACE src)

add_executable(app src/main.cpp)
target_link_libraries(app core)

add_executable(
    unittests
    tests/dfs.cpp
)
target_link_libraries(unittests core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unittests)
