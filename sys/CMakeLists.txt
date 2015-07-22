cmake_minimum_required(VERSION 2.8.9)
project(Kernel)

set(SOURCE
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
	objects/IOArray.cpp
	objects/IOCatalogue.cpp
	objects/IODictionary.cpp
	objects/IONull.cpp
	objects/IONumber.cpp
	objects/IOObject.cpp
	objects/IOSet.cpp
	objects/IOString.cpp
	os/IPC/IPCMessage.cpp
	os/IPC/IPCPort.cpp
	os/IPC/IPCSpace.cpp
	os/IPC/IPCSyscall.cpp
	os/kernel/bootstrapserver.cpp
	os/kernel/kerneltask.cpp
	os/loader/loader.cpp
	os/locks/mutex.cpp
	os/scheduler/smp/smp_scheduler.cpp
	os/scheduler/scheduler.cpp
	os/scheduler/scheduler_syscall.cpp
	os/scheduler/task.cpp
	os/scheduler/thread.cpp
	os/syscall/kerntrapTable.cpp
	os/syscall/syscall.cpp
	os/syscall/syscallTable.cpp
	os/waitqueue.cpp
	os/workqueue.cpp
	personality/pc/pcpersonality.cpp
	personality/personality.cpp
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

set(TARGET_SOURCE
	bootstrap/x86/bootstrap.S)

set(TARGET_ASM_FILES
	bootstrap/x86/bootstrap.S
	libc/sys/x86/spinlock.S
	libc/sys/x86/syscall.S
	libc/setjmp.S
	machine/interrupts/idt.S
	machine/smp/smp_bootstrap.S)

set(TARGET_FILES ${SOURCE} ${TARGET_SOURCE})

# Use the C compiler to do the pre-processing of .S files since CMake doesn't support it
foreach(FILE ${TARGET_FILES})
	
	get_filename_component(EXTENSION ${FILE} EXT)

	if("${EXTENSION}" STREQUAL ".S")
		set_property(SOURCE ${FILE} PROPERTY LANGUAGE C)
	endif()

endforeach()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-sse -mno-mmx -Wall -Wextra -Wno-unused-parameter -pedantic-errors -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -nostdlib -nostdinc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-sse -mno-mmx -Wall -Wextra -Wno-overloaded-virtual -Wno-reorder -Wno-unused-parameter -pedantic-errors -fno-threadsafe-statics -fno-stack-protector -fno-builtin -fno-rtti -fno-exceptions -nostdinc -nostdlib")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_CURRENT_SOURCE_DIR}/Makefile.ld\"")

add_definitions(-DPERSONALITY=1 -DBOOTLOADER=1 -D__KERNEL)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(firedrake ${TARGET_FILES})
