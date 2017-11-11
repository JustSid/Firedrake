cmake_minimum_required(VERSION 2.8.9)
project(libkern)

set(SOURCES
	../../lib/libc/stdio.c
	../../lib/libc/string.c
	../../lib/libc/ipc/ipc_message.c
	../../lib/libc/ipc/ipc_port.c
	../../lib/libc/sys/x86/spinlock.S
	../../lib/libc/sys/spinlock.c
	../../lib/libcpp/new.cpp
	stubs.c)

set(HEADERS
	kmod.h
	libkern.h
	port.h)

set(TARGET_FILES ${SOURCES} ${HEADERS})

mark_assembly_files("${SOURCES}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${lib_SOURCE_DIR})

add_definitions(-D__LIBKERN)

add_library(kern SHARED ${TARGET_FILES})
