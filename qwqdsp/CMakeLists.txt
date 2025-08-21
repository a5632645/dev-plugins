add_library(qwqdsp
    STATIC
        "./source/oouras.cpp"
)
target_include_directories(qwqdsp PUBLIC "./include")
set_target_properties(qwqdsp PROPERTIES CXX_STANDARD 20)

add_subdirectory(playing/raylib)
add_executable(playing
    "./playing/playing.cpp"
    # "./playing/FontRGB.cpp"
    # "./playing/OLEDDisplayRGB.cpp"
    # "./playing/stb_image_write.c"
)
target_link_libraries(playing PUBLIC qwqdsp raylib)
set_target_properties(playing PROPERTIES CXX_STANDARD 20)
