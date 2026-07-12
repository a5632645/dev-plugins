# ------------------------------------------------------------
# plugin shared
# ------------------------------------------------------------

add_library(pluginshared OBJECT
    pluginshared/include/pluginshared/pluginshared.cpp
    pluginshared/include/pluginshared/align_allocator.hpp
    pluginshared/include/pluginshared/bpm_sync_lfo.hpp
    pluginshared/include/pluginshared/bpm_sync_ui.hpp
    pluginshared/include/pluginshared/component.hpp
    pluginshared/include/pluginshared/juce_param_listener.hpp
    pluginshared/include/pluginshared/preset_manager.hpp
    pluginshared/include/pluginshared/preset_panel.hpp
    pluginshared/include/pluginshared/simd.hpp
    pluginshared/include/pluginshared/update_data.hpp
    pluginshared/include/pluginshared/wrap_parameters.hpp
    pluginshared/include/pluginshared/dsp/delay_line_1ch_4time.hpp
    pluginshared/include/pluginshared/dsp/delay_line_multiple.hpp
    pluginshared/include/pluginshared/dsp/delay_line_single.hpp
    pluginshared/include/pluginshared/dsp/one_pole_tpt.hpp
    pluginshared/include/pluginshared/dsp/stereo_iir_hilbert_cpx.hpp
    pluginshared/include/pluginshared/dsp/stereo_iir_hilbert_cpx2.hpp
    pluginshared/include/pluginshared/dsp/table_sine_v4.hpp
)

target_include_directories(pluginshared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/pluginshared/include
)

# ------------------------------------------------------------
# shared resources
# ------------------------------------------------------------
# simd dispatch
# ------------------------------------------------------------

# ------------------------------------------------------------
# plugin
# ------------------------------------------------------------

add_subdirectory(warpcore)
add_subdirectory(empty)
add_subdirectory(green_vocoder)
add_subdirectory(steep_flanger)
add_subdirectory(vital_reverb)
add_subdirectory(vital_chorus)
add_subdirectory(dispersive_delay)
add_subdirectory(deep_phaser)
add_subdirectory(resonator)
# add_subdirectory(ewarp/source)
# add_subdirectory(analog_synth/source)
# add_subdirectory(debugger/source)
