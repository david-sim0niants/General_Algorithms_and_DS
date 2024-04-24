cmake_minimum_required(VERSION 3.13)
project(General_Algorithms_and_Data_Structures CXX)

set(CMAKE_CXX_STANDARD 20)

set(MAIN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/)
add_subdirectory(src)
add_subdirectory(test)
