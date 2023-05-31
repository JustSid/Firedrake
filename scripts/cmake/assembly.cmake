
macro(mark_assembly_files _TARGET_FILES)

    foreach(_FILE ${_TARGET_FILES})

        get_filename_component(_EXTENSION ${_FILE} EXT)

        if("${_EXTENSION}" STREQUAL ".S")
            set_property(SOURCE ${_FILE} PROPERTY LANGUAGE C)
        endif()

    endforeach()

endmacro()
