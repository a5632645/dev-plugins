cmake_minimum_required(VERSION 3.11)
project(qwqdsp)

set(qwqdsp_have_raylib TRUE)
if (NOT TARGET raylib)
    find_package(raylib QUIET)
    if(NOT raylib_FOUND)
        message(STATUS "attemps build are disabled")
        set(qwqdsp_have_raylib FALSE)
    endif()
endif()

find_package(IPP)
set(qwqdsp_have_ipp ${IPP_FOUND})

if(NOT TARGET eigen)
    message(FATAL_ERROR "need a eigen target")
endif()
if (NOT TARGET simde)
    message(FATAL_ERROR "need a simde target")
endif()


include(cmake/warnings.cmake)

add_library(qwqdsp
    STATIC
        "source/oouras.cpp"
        "source/resample_iir.cpp"
        "source/resample_iir_dynamic.cpp"
        "source/fir_design.cpp"
        "source/real_fft.cpp"
        "source/complex_fft.cpp"
        "source/ipp_init.cpp"
)
qwqdsp_set_warning(qwqdsp)

target_include_directories(qwqdsp PUBLIC "./include")

set_target_properties(qwqdsp PROPERTIES CXX_STANDARD 20)

target_link_libraries(qwqdsp PUBLIC Eigen3::Eigen simde)
if (qwqdsp_have_ipp)
    target_link_libraries(qwqdsp PUBLIC IPP::ipps)
    # target_compile_definitions(qwqdsp PUBLIC QWQDSP_HAVE_IPP)
endif()



if (qwqdsp_have_raylib)
    add_executable(playing
        "playing/playing.cpp"
        "playing/slider.cpp"
        # "./playing/FontRGB.cpp"
        # "./playing/OLEDDisplayRGB.cpp"
        # "./playing/stb_image_write.c"
    )
    target_link_libraries(playing PUBLIC qwqdsp raylib)
    set_target_properties(playing PROPERTIES CXX_STANDARD 20)
    qwqdsp_set_warning(playing)

    if (MSVC)
        target_compile_options(playing PUBLIC /arch:AVX2)
    else()
        target_compile_options(playing PUBLIC -mavx2)
    endif()

    add_subdirectory(attemps)
endif()