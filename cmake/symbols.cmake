
macro(target_create_symbolfile _TARGET)
    add_custom_command(
            TARGET ${_TARGET}
            POST_BUILD
            COMMAND objcopy --only-keep-debug $<TARGET_FILE:${_TARGET}> $<TARGET_FILE:${_TARGET}>.sym)
endmacro()
