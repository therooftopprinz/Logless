cmake_minimum_required(VERSION 3.10)

project(logless)

set(CMAKE_CXX_STANDARD 17)

add_executable(test         test/main.cpp)
add_executable(spawner      src/spawner.cpp)

add_library(logless STATIC src/logless/Logger.cpp)

target_include_directories(logless PUBLIC src/)
target_include_directories(spawner PRIVATE src/)

target_link_libraries(test PRIVATE logless pthread)
