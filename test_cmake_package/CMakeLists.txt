cmake_minimum_required(VERSION 3.15)
project(test_cmake_package LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "../build/install")
find_package(logless REQUIRED)

add_executable(test_cmake_package main.cpp)
target_link_libraries(test_cmake_package PRIVATE logless::logless)