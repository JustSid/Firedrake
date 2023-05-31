cmake_minimum_required(VERSION 3.15)
project(libc)

set(SOURCES
	bsd/divdi3.c
	bsd/moddi3.c
	bsd/qdivrem.c
	bsd/udivdi3.c
	bsd/umoddi3.c
	drake/input.c
	ipc/ipc_bootstrap.c
	ipc/ipc_message.c
	ipc/ipc_port.c
	sys/x86/spinlock.S
	sys/x86/syscall.S
	sys/ioctl.c
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
	sys/ioctl.h
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

mark_assembly_files("${SOURCES}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(c-static STATIC ${TARGET_FILES})
add_library(c SHARED ${TARGET_FILES})
