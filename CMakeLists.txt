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
# cpp_simd_detector
# ------------------------------------------------------------
set(CPP_SIMD_DETECTOR_BUILD_EXAMPLES OFF)
add_subdirectory(libs/cpp_simd_detector)

# # ------------------------------------------------------------
# # simde
# # ------------------------------------------------------------
# add_library(simde INTERFACE)
# target_include_directories(simde INTERFACE libs/simde-no-tests)

# ------------------------------------------------------------
# qwqdsp
# ------------------------------------------------------------
set(QWQDSP_BUILD_TESTS OFF)
add_subdirectory(libs/qwqdsp)

if(APPLE)
    message(STATUS "qwqdsp: Using Apple Accelerate")
    target_link_libraries(qwqdsp INTERFACE "-framework Accelerate")
    target_compile_definitions(qwqdsp INTERFACE QWQDSP_HAVE_ACCELERATE)

else()
    # Try system IPP first
    if(UNIX AND NOT DEFINED IPP_DIR)
        set(IPP_DIR "/opt/intel/oneapi/ipp/latest/lib/cmake/ipp")
    endif()

    set(IPP_SHARED OFF)
    find_package(IPP QUIET)

    # CI fallback
    if(NOT IPP_FOUND AND DEFINED BUILD_IN_CI)
        if(WIN32)
            set(IPP_ROOT "$ENV{USERPROFILE}/.nuget/packages/intelipp.static.win-x64/2022.3.0.387")
            set(IPP_INCLUDE_DIR "${IPP_ROOT}/build/native/include")
            set(IPP_LIBRARY_DIR "${IPP_ROOT}/build/native/win-x64")
            set(IPP_LIBRARIES ippsmt ippcoremt ippimt ippcvmt ippvmmt)

        elseif(UNIX)
            set(IPP_ROOT "/opt/intel/oneapi/ipp/latest")
            set(IPP_INCLUDE_DIR "${IPP_ROOT}/include")
            set(IPP_LIBRARY_DIR "${IPP_ROOT}/lib/intel64")
            set(IPP_LIBRARIES ipps ippcore ippi ippcv ippvm)
        endif() 

        if(EXISTS "${IPP_INCLUDE_DIR}")
            set(IPP_FOUND TRUE)
            message(STATUS "qwqdsp: Using CI IPP")
        endif()
    endif()

    if(IPP_FOUND)
        message(STATUS "qwqdsp: Using Intel IPP")

        # Manual IPP
        if(DEFINED IPP_INCLUDE_DIR)
            target_include_directories(qwqdsp INTERFACE
                ${IPP_INCLUDE_DIR}
            )

            target_link_directories(qwqdsp INTERFACE
                ${IPP_LIBRARY_DIR}
            )

            target_link_libraries(qwqdsp INTERFACE
                ${IPP_LIBRARIES}
            )

        # CMake package IPP
        else()
            target_link_libraries(qwqdsp INTERFACE IPP::ipps)
        endif()

        target_compile_definitions(qwqdsp INTERFACE QWQDSP_HAVE_IPP)

    else()
        message(WARNING
            "qwqdsp: IPP not found, using default Ooura implementation"
        )
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
