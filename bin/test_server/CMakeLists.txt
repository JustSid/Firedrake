cmake_minimum_required(VERSION 3.15)
project(test-server)

set(SOURCE main.c)

include_directories(${libc_SOURCE_DIR})

add_executable(test_server.bin ${SOURCE})
target_link_libraries(test_server.bin crt c-static)
