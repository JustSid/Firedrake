cmake_minimum_required(VERSION 2.8.9)
project(Firedrake)

# General compiler/linker flags

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c11 -mno-sse -mno-mmx -Wall -Wextra -pedantic -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -nostdinc -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 -mno-sse -mno-mmx -Wall -Wextra -pedantic -fno-stack-protector -fno-rtti -fno-omit-frame-pointer -fno-exceptions -nostdinc -nostdlib")

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -m elf_i386")

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS  "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Macros

macro(mark_assembly_files _TARGET_FILES)

    foreach(_FILE ${_TARGET_FILES})

        get_filename_component(_EXTENSION ${_FILE} EXT)

        if("${_EXTENSION}" STREQUAL ".S")
            set_property(SOURCE ${_FILE} PROPERTY LANGUAGE C)
        endif()

    endforeach()

endmacro()

# Sanity check

if(NOT (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")))
    message(FATAL_ERROR "Firedrake must be compiled with Clang")
endif()

add_subdirectory("sys")
add_subdirectory("lib")
add_subdirectory("bin")
add_subdirectory("slib")

add_custom_target(remote-build COMMAND /bin/bash ${CMAKE_SOURCE_DIR}/build.sh)
