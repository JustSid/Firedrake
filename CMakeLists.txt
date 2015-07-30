cmake_minimum_required(VERSION 2.8.9)
project(Firedrake)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_subdirectory("sys")
add_subdirectory("lib")
add_subdirectory("bin")
add_subdirectory("slib")

if(NOT (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")))
    message(FATAL_ERROR "Firedrake must be compiled with Clang")
endif()

add_custom_target(remote-build COMMAND /bin/bash ${CMAKE_SOURCE_DIR}/build.sh)
