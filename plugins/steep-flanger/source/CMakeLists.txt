cmake_minimum_required(VERSION 3.22)

set(QWQ_PLUGIN_NAME "SteepFlanger")
project(${QWQ_PLUGIN_NAME} VERSION 0.1.0)

juce_add_plugin(${QWQ_PLUGIN_NAME}
    # VERSION ...                               # Set this if the plugin version is different to the project version
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    # COMPANY_NAME ...                          # Specify the name of the plugin's author
    COMPANY_NAME Mana
    # IS_SYNTH TRUE/FALSE                       # Is this a synth or an effect?
    # IS_SYNTH TRUE
    # NEEDS_MIDI_INPUT TRUE/FALSE               # Does the plugin need midi input?
    # NEEDS_MIDI_INPUT TRUE
    # NEEDS_MIDI_OUTPUT TRUE/FALSE              # Does the plugin need midi output?
    # IS_MIDI_EFFECT TRUE/FALSE                 # Is this plugin a MIDI effect?
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?
    # COPY_PLUGIN_AFTER_BUILD TRUE/FALSE        # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE Mana               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Spfl                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS AU VST3 Standalone                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME ${QWQ_PLUGIN_NAME})        # The name of the final executable, which can differ from the target name

target_sources(${QWQ_PLUGIN_NAME}
    PRIVATE
        PluginEditor.cpp
        PluginProcessor.cpp
        vec4.cpp
        vec8.cpp
)
set_target_properties(${QWQ_PLUGIN_NAME} PROPERTIES CXX_STANDARD 20)
set_source_files_properties(vec4.cpp PROPERTIES COMPILE_OPTIONS ${PLUGIN_VEC4_COMPLIER_OPTION})
set_source_files_properties(vec8.cpp PROPERTIES COMPILE_OPTIONS ${PLUGIN_VEC8_COMPLIER_OPTION})

target_compile_definitions(${QWQ_PLUGIN_NAME}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(${QWQ_PLUGIN_NAME}
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
        Eigen3::Eigen
        qwqdsp
        cpp_simd_detector
        PluginShared
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
