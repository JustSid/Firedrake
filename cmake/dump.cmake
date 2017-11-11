
macro(target_dump _TARGET)
    add_custom_command(
            TARGET ${_TARGET}
            POST_BUILD
            COMMAND objdump -d $<TARGET_FILE:${_TARGET}> > $<TARGET_FILE:${_TARGET}>.dump.txt)
endmacro()

