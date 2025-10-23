cmake_minimum_required(VERSION 4.0)
project(Blockgame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(Dependencies.cmake)

add_subdirectory(Engine)
add_subdirectory(BlockGame)
