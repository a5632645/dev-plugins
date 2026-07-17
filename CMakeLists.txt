cmake_minimum_required(VERSION 3.22)
project(DevPlugins)

# ------------------------------------------------------------
# configs
# ------------------------------------------------------------

option(BUILD_STANDALONE "Build Standalone plugin format" ON)
option(BUILD_VST3 "Build VST3 plugin format" ON)
option(BUILD_LV2 "Build LV2 plugin format" ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Runtime Library")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "Build for 10.13")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------
# juce
# ------------------------------------------------------------
add_subdirectory(libs/JUCE)

# ------------------------------------------------------------
# audiofft
# ------------------------------------------------------------
add_library(audiofft STATIC libs/AudioFFT/AudioFFT.cpp libs/AudioFFT/AudioFFTcpx.cpp)
target_include_directories(audiofft PUBLIC libs/AudioFFT)
set_target_properties(audiofft PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
if(APPLE)
    target_link_libraries(audiofft PRIVATE "-framework Accelerate")
    target_compile_definitions(audiofft PRIVATE AUDIOFFT_APPLE_ACCELERATE)
else()
    if (UNIX)
        set(IPP_DIR "/opt/intel/oneapi/ipp/latest/lib/cmake/ipp")
    endif()
    find_package(IPP QUIET)
    if(NOT IPP_FOUND)
        message(WARNING "AudioFFT: IPP not found, using default Ooura implementation")
    else()
        message(STATUS "AudioFFT: Using Intel IPP")
        target_link_libraries(audiofft PUBLIC ${IPP_LIBRARIES})
        target_compile_definitions(audiofft PRIVATE AUDIOFFT_INTEL_IPP)
    endif()
endif()

# ------------------------------------------------------------
# cpp_simd_detector
# ------------------------------------------------------------
set(CPP_SIMD_DETECTOR_BUILD_EXAMPLES OFF)
add_subdirectory(libs/cpp_simd_detector)

# ------------------------------------------------------------
# simde
# ------------------------------------------------------------
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE libs/simde-no-tests)

# ------------------------------------------------------------
# qwqdsp
# ------------------------------------------------------------
set(QWQDSP_BUILD_TESTS OFF)

if(APPLE)
    set(QWQDSP_USE_ACCELERATE ON)
else()
    if(UNIX)
        # 提示：如果用户在外部设置了 IPP_DIR，保留用户的设置，否则使用默认路径
        if(NOT DEFINED IPP_DIR)
            set(IPP_DIR "/opt/intel/oneapi/ipp/latest/lib/cmake/ipp")
        endif()
    endif()

    find_package(IPP QUIET)
    if(IPP_FOUND)
        set(QWQDSP_USE_IPP ON)
    endif()
endif()

add_subdirectory(libs/qwqdsp)

if(APPLE)
    target_link_libraries(qwqdsp INTERFACE "-framework Accelerate")
else()
    if(IPP_FOUND)
        target_link_libraries(qwqdsp INTERFACE ${IPP_LIBRARIES})
    endif()
endif()

# ------------------------------------------------------------
# nlohmann_json
# ------------------------------------------------------------
add_subdirectory(libs/json)

# ------------------------------------------------------------
# plugins
# ------------------------------------------------------------
add_subdirectory(plugins)
