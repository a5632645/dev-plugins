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

# folders
add_subdirectory(json)
add_subdirectory(eigen)
add_subdirectory(JUCE)
add_subdirectory(raylib)
add_subdirectory(qwqdsp)
add_subdirectory(plugins)