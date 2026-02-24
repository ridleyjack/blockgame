cmake_minimum_required(VERSION 3.25)
project(Blockgame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TESTS "Build unit tests" ON)

include(Dependencies.cmake)

add_subdirectory(Engine)
add_subdirectory(BlockGame)

if (ENABLE_TESTS)
    enable_testing()
endif ()
