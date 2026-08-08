set(PLUGIN_NAME STTR)
set(PLUGIN_VERSION 0.0.1)

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
    PLUGIN_CODE Sttr
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
    src/PluginEditor.cpp
    src/PluginProcessor.cpp
    src/dsp/SttrProcessor.cpp
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
    juce::juce_audio_utils
    qwqdsp
    pluginshared
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ------------------------------------------------------------
# resource
# ------------------------------------------------------------
file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.*")

if(NOT "${res}" STREQUAL "")
    juce_add_binary_data(${PLUGIN_NAME}_res SOURCES ${res})
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${PLUGIN_NAME}_res)
endif()
