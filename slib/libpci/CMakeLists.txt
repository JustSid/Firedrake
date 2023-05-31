cmake_minimum_required(VERSION 3.15)
project(libpci)

set(SOURCES
        IOPCIDevice.cpp
        IOPCIDeviceResource.cpp
        IOPCIProvider.cpp)

set(TARGET_FILES ${SOURCES})

mark_assembly_files("${SOURCES}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${libkern_SOURCE_DIR})
include_directories(${libio_SOURCE_DIR})
include_directories(${lib_SOURCE_DIR})

add_library(PCI SHARED ${TARGET_FILES})
target_link_libraries(PCI io)
