cmake_minimum_required(VERSION 2.8.9)
project(Firedrake)

# General compiler/linker flags

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c11 -mno-sse -mno-mmx -Wall -Wextra -pedantic -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 -mno-sse -mno-mmx -Wall -Wextra -pedantic -fno-stack-protector -fno-rtti -fno-omit-frame-pointer -fno-exceptions -nostdlib")

set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -m elf_i386")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}  -Wl,--hash-style=sysv")

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS  "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Macros
include(cmake/symbols.cmake)
include(cmake/assembly.cmake)
include(cmake/dump.cmake)

set(libcpp_SOURCE_DIR "${Firedrake_SOURCE_DIR}/lib/libcpp")
set(lib_SOURCE_DIR "${Firedrake_SOURCE_DIR}/lib")
set(slib_SOURCE_DIR "${Firedrake_SOURCE_DIR}/slib")


# Add targets
add_subdirectory("sys")
add_subdirectory("lib")
add_subdirectory("bin")
add_subdirectory("slib")
