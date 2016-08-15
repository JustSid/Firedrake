cmake_minimum_required(VERSION 2.8.9)
project(init)

set(SOURCE main.cpp)

include_directories(${libc_SOURCE_DIR})

add_executable(init.bin ${SOURCE})
target_link_libraries(init.bin crt c-static)
