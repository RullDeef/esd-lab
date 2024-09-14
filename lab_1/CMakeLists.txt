cmake_minimum_required(VERSION 3.10)

project(ExpertSystemDesignLabs)

add_executable(
    app
    src/dict.cpp
    src/graph_search.cpp
    src/graph_viz.cpp
    src/main.cpp
    src/rule_printer.cpp
)
