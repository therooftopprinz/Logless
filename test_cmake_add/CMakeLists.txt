cmake_minimum_required(VERSION 3.15)
project(test_cmake_add LANGUAGES CXX)

add_subdirectory(logless)

add_executable(test_cmake_add main.cpp)
target_link_libraries(test_cmake_add PRIVATE logless)