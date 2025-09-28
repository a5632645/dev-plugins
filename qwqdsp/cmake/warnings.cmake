function(qwqdsp_set_warning _target)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${_target}
        PRIVATE
            -Wall
            -Wextra
            -Wno-newline-eof
            -Wno-c++98-compat-pedantic
            -Wno-c++98-compat
            -Wno-c++11-compat
            -Wno-c++17-compat
            -Wno-unsafe-buffer-usage
            -Wno-documentation-unknown-command
    )
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    endif()
endfunction()
