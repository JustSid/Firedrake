cmake_minimum_required(VERSION 3.15)
project(Firedrake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

find_package(Python COMPONENTS Interpreter)

# General compiler/linker flags
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -mno-sse -mno-mmx -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -mno-sse -mno-mmx -fno-use-cxa-atexit -fno-stack-protector -fno-rtti -fno-omit-frame-pointer -fno-exceptions -nostdlib")

set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -m elf_i386 -z noexecstack")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}  -Wl,--hash-style=sysv -z noexecstack")

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS  "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Macros
include(scripts/cmake/symbols.cmake)
include(scripts/cmake/assembly.cmake)
include(scripts/cmake/dump.cmake)

set(libcpp_SOURCE_DIR "${Firedrake_SOURCE_DIR}/lib/libcpp")
set(lib_SOURCE_DIR "${Firedrake_SOURCE_DIR}/lib")
set(slib_SOURCE_DIR "${Firedrake_SOURCE_DIR}/slib")

# Add targets
add_subdirectory("sys")
add_subdirectory("lib")
add_subdirectory("bin")
add_subdirectory("slib")

# Phony target to make a full iso
add_custom_target(firedrake_iso
	DEPENDS firedrake init.bin mishell.bin term.bin test.bin test_server.bin HID io kern PCI QEMU
	COMMAND Python::Interpreter "${Firedrake_SOURCE_DIR}/scripts/initrd.py"
	COMMAND Python::Interpreter "${Firedrake_SOURCE_DIR}/scripts/make_image.py")
