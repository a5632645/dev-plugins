set(PLUGIN_NAME WarpCore)
set(PLUGIN_VERSION 0.0.13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# ------------------------------------------------------------
# plugin formats
# ------------------------------------------------------------
set(plugin_formats AU)

if(BUILD_STANDALONE)
    list(APPEND plugin_formats Standalone)
endif()

if(BUILD_VST3)
    list(APPEND plugin_formats VST3)
endif()

if(BUILD_LV2 AND UNIX AND NOT APPLE)
    list(APPEND plugin_formats LV2)
endif()

# ----------------------------------------
# plugin info
# ----------------------------------------
juce_add_plugin(${PLUGIN_NAME}
    VERSION ${PLUGIN_VERSION}
    COMPANY_NAME "ManasWolrd"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT False
    NEEDS_MIDI_OUTPUT false
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    JUCE_VST3_CAN_REPLACE_VST2 FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Mana
    PLUGIN_CODE Warp
    FORMATS ${plugin_formats}
    PRODUCT_NAME "${PLUGIN_NAME}"
    BUNDLE_ID "com.ManasWolrd.${PLUGIN_NAME}"
    LV2URI "https://github.com/ManasWolrd/dev-plugins"
)

# ------------------------------------------------------------
# plugin code
# ------------------------------------------------------------
target_sources(${PLUGIN_NAME}
    PRIVATE
    src/ui/plugin_ui.cpp
    src/PluginProcessor.cpp
    src/dsp/dispatch.cpp
)

target_include_directories(${PLUGIN_NAME}
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(${PLUGIN_NAME}
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

if(APPLE)
    target_compile_definitions(${PLUGIN_NAME} PUBLIC JUCE_AU=1)
endif()

target_link_libraries(${PLUGIN_NAME}
    PRIVATE
    juce::juce_dsp
    juce::juce_core
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_audio_utils
    pluginshared
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    cpp_simd_detector
)

# ------------------------------------------------------------
# resource
# ------------------------------------------------------------
file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.*")

if(NOT "${res}" STREQUAL "")
    juce_add_binary_data(${PLUGIN_NAME}_res SOURCES ${res})
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${PLUGIN_NAME}_res)
endif()

# ------------------------------------------------------------
# simd dispatch
# ------------------------------------------------------------
if(APPLE)
    # Collect target architectures for SIMD file dispatch.
    # CMAKE_OSX_ARCHITECTURES is one of: x86_64 / arm64 / x86_64;arm64
    if(CMAKE_OSX_ARCHITECTURES)
        set(_archs ${CMAKE_OSX_ARCHITECTURES})
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(_archs "arm64")
    else()
        set(_archs "x86_64")
    endif()

    if(_archs STREQUAL "x86_64")
        target_sources(${PLUGIN_NAME}
            PRIVATE
            src/dsp/dsp_avx2.cpp
            src/dsp/dsp_avx.cpp
            src/dsp/dsp_sse4.cpp
            src/dsp/dsp_sse2.cpp
            src/dsp/dsp_avx2_fma.cpp
        )
        set_source_files_properties("src/dsp/dsp_sse2.cpp" PROPERTIES COMPILE_FLAGS "-msse2 -ffast-math -Wno-nan-infinity-disabled")
        set_source_files_properties("src/dsp/dsp_sse4.cpp" PROPERTIES COMPILE_FLAGS "-msse4.1 -ffast-math -Wno-nan-infinity-disabled")
        set_source_files_properties("src/dsp/dsp_avx.cpp" PROPERTIES COMPILE_FLAGS "-mavx -ffast-math -Wno-nan-infinity-disabled")
        set_source_files_properties("src/dsp/dsp_avx2.cpp" PROPERTIES COMPILE_FLAGS "-mavx2 -ffast-math -Wno-nan-infinity-disabled")
        set_source_files_properties("src/dsp/dsp_avx2_fma.cpp" PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -ffast-math -Wno-nan-infinity-disabled")

    elseif(_archs STREQUAL "arm64")
        target_sources(${PLUGIN_NAME} PRIVATE src/dsp/dsp_neon.cpp)
        set_source_files_properties("src/dsp/dsp_neon.cpp" PROPERTIES COMPILE_FLAGS "-ffast-math -Wno-nan-infinity-disabled")

    else()
        # universal binary: x86_64 + arm64
        target_sources(${PLUGIN_NAME} PRIVATE src/dsp/dsp_neon.cpp)
        set_source_files_properties("src/dsp/dsp_neon.cpp" PROPERTIES COMPILE_FLAGS "-arch arm64 -ffast-math -Wno-nan-infinity-disabled")

        target_sources(${PLUGIN_NAME}
            PRIVATE
            src/dsp/dsp_avx2.cpp
            src/dsp/dsp_avx.cpp
            src/dsp/dsp_sse4.cpp
            src/dsp/dsp_sse2.cpp
            src/dsp/dsp_avx2_fma.cpp
        )
        set_source_files_properties("src/dsp/dsp_sse2.cpp" PROPERTIES COMPILE_FLAGS "-msse2 -ffast-math -Wno-nan-infinity-disabled")
        set_source_files_properties("src/dsp/dsp_sse4.cpp" PROPERTIES COMPILE_FLAGS "-msse4.1 -ffast-math -Wno-nan-infinity-disabled")
        set_source_files_properties("src/dsp/dsp_avx.cpp" PROPERTIES COMPILE_FLAGS "-mavx -ffast-math -Wno-nan-infinity-disabled")
        set_source_files_properties("src/dsp/dsp_avx2.cpp" PROPERTIES COMPILE_FLAGS "-mavx2 -ffast-math -Wno-nan-infinity-disabled")
        set_source_files_properties("src/dsp/dsp_avx2_fma.cpp" PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -ffast-math -Wno-nan-infinity-disabled")
    endif()

    unset(_archs)
else()
    target_sources(${PLUGIN_NAME}
        PRIVATE
        src/dsp/dsp_avx2_fma.cpp
        src/dsp/dsp_avx2.cpp
        src/dsp/dsp_avx.cpp
        src/dsp/dsp_sse4.cpp
        src/dsp/dsp_sse2.cpp
    )
    set_source_files_properties("src/dsp/dsp_sse2.cpp" PROPERTIES COMPILE_FLAGS "-msse2 -ffast-math -Wno-nan-infinity-disabled")
    set_source_files_properties("src/dsp/dsp_sse4.cpp" PROPERTIES COMPILE_FLAGS "-msse4.1 -ffast-math -Wno-nan-infinity-disabled")
    set_source_files_properties("src/dsp/dsp_avx.cpp" PROPERTIES COMPILE_FLAGS "-mavx -ffast-math -Wno-nan-infinity-disabled")
    set_source_files_properties("src/dsp/dsp_avx2.cpp" PROPERTIES COMPILE_FLAGS "-mavx2 -ffast-math -Wno-nan-infinity-disabled")
    set_source_files_properties("src/dsp/dsp_avx2_fma.cpp" PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -ffast-math -Wno-nan-infinity-disabled")
endif()
