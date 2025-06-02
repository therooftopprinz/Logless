cmake_minimum_required(VERSION 3.15)

project(logless VERSION 1.0.39 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Ofast")

add_library(logless STATIC src/logless/logger.cpp)
target_include_directories(logless PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

add_executable(test         test/main.cpp)
add_executable(spawner      src/spawner.cpp)

target_link_libraries(spawner PRIVATE logless)
target_link_libraries(test PRIVATE logless pthread)

install(
    TARGETS logless spawner
    EXPORT loglessTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install(
    FILES src/logless/logger.hpp
    DESTINATION include/logless)

install(
    EXPORT loglessTargets
    FILE loglessTargets.cmake
    NAMESPACE logless::
    DESTINATION lib/cmake/logless)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/loglessConfig.cmake
    INSTALL_DESTINATION lib/cmake/logless)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/loglessConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/loglessConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/loglessConfigVersion.cmake
    DESTINATION lib/cmake/logless)
