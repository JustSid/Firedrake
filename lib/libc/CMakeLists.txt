cmake_minimum_required(VERSION 2.8.9)
project(libc)

set(SOURCES
	bsd/divdi3.c
	bsd/moddi3.c
	bsd/qdivrem.c
	bsd/udivdi3.c
	bsd/umoddi3.c
	ipc/ipc_bootstrap.c
	ipc/ipc_message.c
	ipc/ipc_port.c
	sys/x86/spinlock.S
	sys/x86/syscall.S
	sys/mman.c
	sys/thread.c
	sys/tls.c
	sys/unistd.c
	backtrace.c
	setjmp.S
	stdio.c
	stdlib.c
	string.c)

# Use the C compiler to do the pre-processing of .S files since CMake doesn't support it
foreach(FILE ${SOURCES})

	get_filename_component(EXTENSION ${FILE} EXT)

	if("${EXTENSION}" STREQUAL ".S")
		set_property(SOURCE ${FILE} PROPERTY LANGUAGE C)
	endif()

endforeach()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(c-static STATIC ${SOURCES})
add_library(c SHARED ${SOURCES})
