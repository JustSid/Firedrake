cmake_minimum_required(VERSION 2.8.9)
project(libqemu)

set(SOURCES
        QMModule.cpp
        QMVBEDevice.cpp)

set(TARGET_FILES ${SOURCES})

# Use the C compiler to do the pre-processing of .S files since CMake doesn't support it
foreach(FILE ${TARGET_FILES})

    get_filename_component(EXTENSION ${FILE} EXT)

    if("${EXTENSION}" STREQUAL ".S")
        set_property(SOURCE ${FILE} PROPERTY LANGUAGE C)
    endif()

endforeach()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${libkern_SOURCE_DIR})
include_directories(${libio_SOURCE_DIR})
include_directories(${libpci_SOURCE_DIR})

add_library(QEMU SHARED ${TARGET_FILES})
target_link_libraries(QEMU io PCI)
