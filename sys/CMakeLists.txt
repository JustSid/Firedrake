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
	../lib/libc/bsd/divdi3.c
	../lib/libc/bsd/moddi3.c
	../lib/libc/bsd/qdivrem.c
	../lib/libc/bsd/udivdi3.c
	../lib/libc/bsd/umoddi3.c
	../lib/libc/sys/x86/spinlock.S
	../lib/libc/sys/spinlock.c
	../lib/libc/backtrace.c
	../lib/libc/setjmp.S
	../lib/libc/stdio.c
	../lib/libc/stdlib.c
	../lib/libc/string.c
	../lib/libcpp/cxa.cpp
	../lib/libcpp/new.cpp
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
	../slib/libio/core/IOArray.cpp
	../slib/libio/core/IOCatalogue.cpp
	../slib/libio/core/IODictionary.cpp
	../slib/libio/core/IONull.cpp
	../slib/libio/core/IONumber.cpp
	../slib/libio/core/IOObject.cpp
	../slib/libio/core/IORegistry.cpp
	../slib/libio/core/IOSet.cpp
	../slib/libio/core/IOString.cpp
	../slib/libio/video/IODisplay.cpp
	../slib/libio/video/IOFramebuffer.cpp
	os/ipc/IPCMessage.cpp
	os/ipc/IPCPort.cpp
	os/ipc/IPCSpace.cpp
	os/ipc/IPCSyscall.cpp
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
	os/waitqueue.cpp
	os/workqueue.cpp
	${CONFIG_PERSONALITY_PATH}/personality.cpp
	personality/personality.cpp
	vfs/cfs/cfs_descriptor.cpp
	vfs/cfs/cfs_instance.cpp
	vfs/cfs/cfs_node.cpp
	vfs/devfs/devices.cpp
	vfs/devfs/framebuffer.cpp
	vfs/devfs/keyboard.cpp
	vfs/devfs/pty.cpp
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
	machine/interrupts/idt.h
	machine/memory/memory.h
	machine/cme.h
	machine/cmos.h
	machine/port.h
	os/ipc/IPC.h
	os/loader/elf.h
	os/interruptguard.h
	prefix.h
	version.h)

set(TARGET_SOURCES
	bootstrap/x86/bootstrap.S)

set(TARGET_HEADERS
	machine/interrupts/x86/interrupts.h)

set(TARGET_FILES ${SOURCES} ${TARGET_SOURCES} ${HEADERS} ${TARGET_HEADERS})

mark_assembly_files("${TARGET_FILES}")

# Compiler settings

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_CURRENT_SOURCE_DIR}/Makefile.ld\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

add_definitions(-D__KERNEL)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${lib_SOURCE_DIR})
include_directories(${slib_SOURCE_DIR})

add_executable(firedrake ${TARGET_FILES})
target_dump(firedrake)
target_create_symbolfile(firedrake)
