cmake_minimum_required(VERSION 2.8.9)
project(Kernel)

# Configuration

set(CONFIG_MAX_CPUS "32" CACHE STRING "The maximum number of CPUs supported by the kernel")

set(CONFIG_PERSONALITY_PATH "personality/pc" CACHE PATH "Path to the personality")
set(CONFIG_PERSONALITY_HEADER "<${CONFIG_PERSONALITY_PATH}/personality.h>")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)

# Source files

set(SOURCES
	bootstrap/boot.cpp
	kern/kalloc.cpp
	kern/kern_return.cpp
	kern/kprintf.cpp
	kern/panic.cpp
	libc/bsd/divdi3.c
	libc/bsd/moddi3.c
	libc/bsd/qdivrem.c
	libc/bsd/udivdi3.c
	libc/bsd/umoddi3.c
	libc/sys/x86/spinlock.S
	libc/sys/spinlock.c
	libc/backtrace.c
	libc/setjmp.S
	libc/stdio.c
	libc/stdlib.c
	libc/string.c
	libcpp/cxa.cpp
	libcpp/new.cpp
	machine/clock/clock.cpp
	machine/interrupts/apic.cpp
	machine/interrupts/idt.S
	machine/interrupts/interrupts.cpp
	machine/interrupts/trampoline.cpp
	machine/memory/heap.cpp
	machine/memory/physical.cpp
	machine/memory/virtual.cpp
	machine/smp/smp.cpp
	machine/smp/smp_bootstrap.S
	machine/acpi.cpp
	machine/cpu.cpp
	machine/debug.cpp
	machine/gdt.cpp
	libio/core/IOArray.cpp
	libio/core/IOCatalogue.cpp
	libio/core/IODictionary.cpp
	libio/core/IONull.cpp
	libio/core/IONumber.cpp
	libio/core/IOObject.cpp
	libio/core/IORegistry.cpp
	libio/core/IOSet.cpp
	libio/core/IOString.cpp
	libio/video/IODisplay.cpp
	libio/video/IOFramebuffer.cpp
	vfs/devfs/devices.cpp
	vfs/devfs/framebuffer.cpp
	vfs/devfs/keyboard.cpp
	os/IPC/IPCMessage.cpp
	os/IPC/IPCPort.cpp
	os/IPC/IPCSpace.cpp
	os/IPC/IPCSyscall.cpp
	os/kernel/bootstrapserver.cpp
	os/kernel/kerneltask.cpp
	os/linker/LDLibkernModule.cpp
	os/linker/LDModule.cpp
	os/linker/LDService.cpp
	os/linker/LDStore.cpp
	os/loader/loader.cpp
	os/locks/mutex.cpp
	os/scheduler/smp/smp_scheduler.cpp
	os/scheduler/scheduler.cpp
	os/scheduler/scheduler_syscall.cpp
	os/scheduler/task.cpp
	os/scheduler/thread.cpp
	os/syscall/kerntrapTable.cpp
	os/syscall/syscall.cpp
	os/syscall/syscall_mmap.cpp
	os/syscall/syscallTable.cpp
	os/pty.cpp
	os/waitqueue.cpp
	os/workqueue.cpp
	${CONFIG_PERSONALITY_PATH}/personality.cpp
	personality/personality.cpp
	vfs/cfs/cfs_descriptor.cpp
	vfs/cfs/cfs_instance.cpp
	vfs/cfs/cfs_node.cpp
	vfs/ffs/ffs_descriptor.cpp
	vfs/ffs/ffs_instance.cpp
	vfs/ffs/ffs_node.cpp
	vfs/context.cpp
	vfs/descriptor.cpp
	vfs/file.cpp
	vfs/instance.cpp
	vfs/node.cpp
	vfs/path.cpp
	vfs/vfs.cpp
	vfs/vfs_syscall.cpp)

# Needed for CLion only so they are recognized correctly as part of the project
# Not actually needed for pure CMake

set(HEADERS
	bootstrap/multiboot.h
	libc/assert.h
	libc/math.h
	libc/stdatomic.h
	libc/stdarg.h
	libc/stdbool.h
	libc/stddef.h
	libc/stdint.h
	libcpp/algorithm.h
	libcpp/atomic.h
	libcpp/bitfield.h
	libcpp/functional.h
	libcpp/intrusive_list.h
	libcpp/iterator.h
	libcpp/type_traits.h
	libcpp/vector.h
	libio/core/IOFunction.h
	libio/core/IOHashtableInternal.h
	libio/core/IORuntime.h
	libio/hid/IOHIDKeyboardUtilities.h
	machine/interrupts/idt.h
	machine/memory/memory.h
	machine/cme.h
	machine/cmos.h
	machine/port.h
	os/ipc/IPC.h
	os/loader/elf.h
	prefix.h
	version.h)

set(TARGET_SOURCES
	bootstrap/x86/bootstrap.S)

set(TARGET_HEADERS
	machine/interrupts/x86/interrupts.h)

set(TARGET_FILES ${SOURCES} ${TARGET_SOURCES} ${HEADERS} ${TARGET_HEADERS})

# Use the C compiler to do the pre-processing of .S files since CMake doesn't support it
mark_assembly_files("${TARGET_FILES}")

# Compiler settings

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_CURRENT_SOURCE_DIR}/Makefile.ld\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

add_definitions(-D__KERNEL)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_BINARY_DIR}/include)

add_executable(firedrake ${TARGET_FILES})
