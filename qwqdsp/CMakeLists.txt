add_library(qwqdsp
    STATIC
        "./source/oouras.cpp"
)
target_include_directories(qwqdsp PUBLIC "./include")
set_target_properties(qwqdsp PROPERTIES CXX_STANDARD 20)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(qwqdsp
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

if (qwqdsp_have_raylib)
    add_executable(playing
        "./playing/playing.cpp"
        "./playing/slider.cpp"
        # "./playing/FontRGB.cpp"
        # "./playing/OLEDDisplayRGB.cpp"
        # "./playing/stb_image_write.c"
    )
    target_link_libraries(playing PUBLIC qwqdsp raylib)
    set_target_properties(playing PROPERTIES CXX_STANDARD 20)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(playing
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

    add_subdirectory(examples)
endif()