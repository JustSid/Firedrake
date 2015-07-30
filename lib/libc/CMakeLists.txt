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
	sys/spinlock.c
	sys/task.c
	sys/thread.c
	sys/tls.c
	sys/unistd.c
	backtrace.c
	setjmp.S
	stdio.c
	stdlib.c
	string.c)

set(HEADERS
	ipc/ipc_types.h
	sys/asm.h
	sys/cdefs.h
	sys/dirent.h
	sys/errno.h
	sys/fcntl.h
	sys/kern_return.h
	sys/kern_trap.h
	sys/syscall.h
	sys/types.h
	assert.h
	math.h
	setjmp.h
	stdarg.h
	stdatomic.h
	stdbool.h
	stddef.h
	stdint.h)

set(TARGET_FILES ${SOURCES} ${HEADERS})

# Use the C compiler to do the pre-processing of .S files since CMake doesn't support it
foreach(FILE ${TARGET_FILES})

	get_filename_component(EXTENSION ${FILE} EXT)

	if("${EXTENSION}" STREQUAL ".S")
		set_property(SOURCE ${FILE} PROPERTY LANGUAGE C)
	endif()

endforeach()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(c-static STATIC ${TARGET_FILES})
add_library(c SHARED ${TARGET_FILES})
