cmake_minimum_required(VERSION 3.22)
project(DevPlugins)

# this is always be true
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# msvc utf-8 strings
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# simde
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE simde-no-tests)

# complier option for dynamic simd dispatch
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if (MSVC)
        message(STATUS FATAL_ERROR "msvc on macos?")
    else()
        set(PLUGIN_VEC4_COMPLIER_OPTION -march=native)
        set(PLUGIN_VEC4_LESS_COMPLIER_OPTION -march=native)
        set(PLUGIN_VEC8_COMPLIER_OPTION -march=native)
        set(PLUGIN_VEC8_LESS_COMPLIER_OPTION -march=native)
        add_compile_definitions(PLUGIN_VEC4_DISPATCH_ISET=NEON)
        add_compile_definitions(PLUGIN_VEC4_LESS_DISPATCH_ISET=NEON)
        add_compile_definitions(PLUGIN_VEC8_DISPATCH_ISET=NEON)
        add_compile_definitions(PLUGIN_VEC8_LESS_DISPATCH_ISET=NEON)
    endif()
else()
    if (MSVC)
        set(PLUGIN_VEC4_COMPLIER_OPTION /arch:SSE4.2)
        set(PLUGIN_VEC4_LESS_COMPLIER_OPTION /arch:SSE2)
        set(PLUGIN_VEC8_COMPLIER_OPTION /arch:avx2)
        set(PLUGIN_VEC8_LESS_COMPLIER_OPTION /arch:avx)
    else()
        set(PLUGIN_VEC4_COMPLIER_OPTION -msse4.1)
        set(PLUGIN_VEC4_LESS_COMPLIER_OPTION -msse4.1)
        set(PLUGIN_VEC8_COMPLIER_OPTION -mavx2)
        set(PLUGIN_VEC8_LESS_COMPLIER_OPTION -mavx)
    endif()
    # basic sse support
    add_compile_definitions(PLUGIN_VEC4_LESS_DISPATCH_ISET=SSE2)
    # most sse support
    add_compile_definitions(PLUGIN_VEC4_DISPATCH_ISET=SSE4_1)
    # most avx2 support
    add_compile_definitions(PLUGIN_VEC8_DISPATCH_ISET=AVX2)
    # if a plugin don't use avx2 intergal instruction, use this
    add_compile_definitions(PLUGIN_VEC8_LESS_DISPATCH_ISET=AVX)
endif()

# floating point optimise
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -ffast-math
        -fno-finite-math-only
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -ffast-math
        -fno-finite-math-only
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
        /fp:fast # msvc won't delete std::isnan std::isinf
    )
endif()

# folders
add_subdirectory(cpp_simd_detector)
add_subdirectory(json)
add_subdirectory(eigen)
add_subdirectory(JUCE)
add_subdirectory(raylib)
add_subdirectory(qwqdsp)
add_subdirectory(plugins)

include(get_cmake_target_info.cmake)
