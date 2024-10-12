cmake_minimum_required(VERSION 3.16.3)
project(markow_word_gen)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
add_executable(markow_word_gen main.cpp)
target_include_directories(markow_word_gen PUBLIC ${PROJECT_SOURCE_DIR})