cmake_minimum_required(VERSION 2.8.9)
project(libkern)

set(SOURCES
	libc/stdio.c
	libc/string.c
	libc/ipc/ipc_message.c
	libc/ipc/ipc_port.c
	libc/sys/x86/spinlock.S
	libc/sys/spinlock.c
	libcpp/new.cpp
	stubs.c)

set(HEADERS
	libc/ipc/ipc_types.h
	libc/sys/cdefs.h
	libc/sys/spinlock.h
	libc/sys/types.h
	libc/stdarg.h
	libc/stdatomic.h
	libc/stdbool.h
	libc/stddef.h
	libc/stdint.h
	libc/stdio.h
	libc/string.h
	libcpp/algorithm.h
	libcpp/atomic.h
	libcpp/bitfield.h
	libcpp/functional.h
	libcpp/intrusive_list.h
	libcpp/iterator.h
	libcpp/type_traits.h
	libcpp/vector.h
	kmod.h
	libkern.h
	port.h)

set(TARGET_FILES ${SOURCES} ${HEADERS})

# Use the C compiler to do the pre-processing of .S files since CMake doesn't support it
foreach(FILE ${TARGET_FILES})

	get_filename_component(EXTENSION ${FILE} EXT)

	if("${EXTENSION}" STREQUAL ".S")
		set_property(SOURCE ${FILE} PROPERTY LANGUAGE C)
	endif()

endforeach()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-D__LIBKERN)

add_library(kern SHARED ${TARGET_FILES})
