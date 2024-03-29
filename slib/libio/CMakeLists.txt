cmake_minimum_required(VERSION 3.15)
project(libio)

set(SOURCES
	core/IOArray.cpp
	core/IOCatalogue.cpp
	core/IODictionary.cpp
	core/IOMemoryRegion.cpp
	core/IONull.cpp
	core/IONumber.cpp
	core/IOObject.cpp
	core/IORegistry.cpp
	core/IORuntime.cpp
	core/IOSet.cpp
	core/IOString.cpp
	service/IOConnector.cpp
	service/IOModule.cpp
	service/IOResources.cpp
	service/IOService.cpp
	service/IOThread.cpp
	hid/IOHIDKeyboardService.cpp
	video/IODisplay.cpp
	video/IOFramebuffer.cpp)

set(HEADERS
	core/IOArray.h
	core/IOCatalogue.h
	core/IODictionary.h
	core/IOFunction.h
	core/IOHashtableInternal.h
	core/IONull.h
	core/IONumber.h
	core/IOObject.h
	core/IORegistry.h
	core/IORuntime.h
	core/IOSet.h
	core/IOString.h
	service/IOConnector.h
	service/IOModule.h
	service/IOResources.cpp
	service/IOService.h
	service/IOThread.h
	hid/IOHIDKeyboardService.h
	hid/IOHIDKeyboardUtilities.h)

set(TARGET_FILES ${SOURCES} ${HEADERS})

mark_assembly_files("${SOURCES}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-sse -mno-mmx -Wno-unused-parameter -fno-stack-protector -fno-omit-frame-pointer -fno-builtin -nostdlib -nostdinc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-sse -mno-mmx -Wno-overloaded-virtual -Wno-reorder -Wno-unused-parameter -fno-use-cxa-atexit -fno-threadsafe-statics -fno-stack-protector -fno-builtin -fno-rtti -fno-exceptions -nostdinc -nostdlib")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${libkern_SOURCE_DIR})
include_directories(${lib_SOURCE_DIR})

add_library(io SHARED ${TARGET_FILES})
target_link_libraries(io kern)

