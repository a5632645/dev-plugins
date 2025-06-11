cmake_minimum_required(VERSION 3.22)

project(LPCVocoder)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_subdirectory(eigen)
add_subdirectory(JUCE)
add_subdirectory(src)
