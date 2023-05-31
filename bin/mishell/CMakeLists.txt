cmake_minimum_required(VERSION 3.15)
project(mishell)

set(SOURCE main.cpp)

include_directories(${libc_SOURCE_DIR})

add_executable(mishell.bin ${SOURCE})
target_link_libraries(mishell.bin crt c-static)
