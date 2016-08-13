cmake_minimum_required(VERSION 2.8.9)
project(term)

set(SOURCES
		iso_font.cpp
		console.cpp
		framebuffer.h
		main.cpp)

include_directories(${libc_SOURCE_DIR})
include_directories(${libc_SOURCE_DIR}/../libcpp)

add_executable(term.bin ${SOURCES})
target_link_libraries(term.bin crt c-static)
