cmake_minimum_required(VERSION 2.6)
project(algorithm)

set(CMAKE_C_FLAGS "-Wall")

option(BUILD_TESTS "Build tests." TRUE)
if (BUILD_TESTS)
enable_testing()
add_subdirectory(tests)
endif()

add_subdirectory(src)
