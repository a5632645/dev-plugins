cmake_minimum_required(VERSION 3.11)
project(qwqdsp)

# check for raylib
set(qwqdsp_have_raylib TRUE)
if (NOT TARGET raylib)
    find_package(raylib QUIET)
    if(NOT raylib_FOUND)
        message(STATUS "tests build are disabled")
        set(qwqdsp_have_raylib FALSE)
    endif()
endif()

# check for intel ipp
set(IPP_SHARED OFF)
find_package(IPP)
set(qwqdsp_have_ipp ${IPP_FOUND})

# set warnings
include(cmake/warnings.cmake)

# main library
add_library(qwqdsp
    STATIC
        "source/oouras.cpp"
        "source/real_fft.cpp"
        "source/complex_fft.cpp"
)
qwqdsp_set_warning(qwqdsp)
target_include_directories(qwqdsp PUBLIC "./include")
set_target_properties(qwqdsp PROPERTIES CXX_STANDARD 20)

# external library
if (TARGET eigen)
    target_link_libraries(qwqdsp PUBLIC Eigen3::Eigen)
    target_compile_definitions(qwqdsp PUBLIC QWQDSP_HAVE_EIGEN=1)
endif()
if (qwqdsp_have_ipp)
    target_link_libraries(qwqdsp PUBLIC IPP::ipps)
    target_compile_definitions(qwqdsp PUBLIC QWQDSP_HAVE_IPP=1)
endif()

# enable tests
if (qwqdsp_have_raylib)
    add_executable(playing
        "playing/playing.cpp"
        "playing/slider.cpp"
        # "playing/miniaudio.c"
    )
    target_link_libraries(playing PUBLIC qwqdsp raylib)
    set_target_properties(playing PROPERTIES CXX_STANDARD 20)
    qwqdsp_set_warning(playing)

    if (MSVC)
        target_compile_options(playing PUBLIC /arch:AVX2)
    else()
        target_compile_options(playing PUBLIC -mavx2)
    endif()

    add_subdirectory(tests)
endif()