cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")

add_subdirectory("libcrt")
add_subdirectory("libc")
