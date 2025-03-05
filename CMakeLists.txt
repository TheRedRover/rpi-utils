cmake_minimum_required(VERSION 3.10)
project(temp-hum-clock)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(addons)
add_subdirectory(temp-hum-clock)
add_subdirectory(utils)