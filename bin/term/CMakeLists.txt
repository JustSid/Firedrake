cmake_minimum_required(VERSION 2.8.9)
project(term)

set(SOURCES
		iso_font.cpp
		main.cpp)

include_directories(${libc_SOURCE_DIR})

add_executable(term.bin ${SOURCES})
target_link_libraries(term.bin crt c-static)
