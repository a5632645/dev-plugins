cmake_minimum_required(VERSION 3.22)

project(GreenVocoder)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

option(GREEN_VOCODER_ENABLE_PERFORMANCE_DEBUG "enable the buildin process time displayer" OFF)

add_subdirectory(eigen)
add_subdirectory(JUCE)
add_subdirectory(qwqdsp)
add_subdirectory(src)
# add_subdirectory(synth)
add_subdirectory(lattice_chorus)
add_subdirectory(freq_shifter)