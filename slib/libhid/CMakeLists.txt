cmake_minimum_required(VERSION 3.15)
project(libhid)

set(SOURCES
	IOHIDModule.cpp
	IOHIDPS2KeyboardService.cpp)

set(TARGET_FILES ${SOURCES})

mark_assembly_files("${SOURCES}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${libkern_SOURCE_DIR})
include_directories(${libio_SOURCE_DIR})
include_directories(${lib_SOURCE_DIR})

add_library(HID SHARED ${TARGET_FILES})
target_link_libraries(HID io)
