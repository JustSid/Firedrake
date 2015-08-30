cmake_minimum_required(VERSION 2.8.9)
project(libio)

set(SOURCES
	core/IOArray.cpp
	core/IOCatalogue.cpp
	core/IODictionary.cpp
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
	hid/IOHIDKeyboardService.cpp)

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

# Use the C compiler to do the pre-processing of .S files since CMake doesn't support it
foreach(FILE ${TARGET_FILES})

	get_filename_component(EXTENSION ${FILE} EXT)

	if("${EXTENSION}" STREQUAL ".S")
		set_property(SOURCE ${FILE} PROPERTY LANGUAGE C)
	endif()

endforeach()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${libkern_SOURCE_DIR})

add_library(io SHARED ${TARGET_FILES})
target_link_libraries(io kern)
