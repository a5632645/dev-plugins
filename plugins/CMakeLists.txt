# ------------------------------------------------------------
# plugin shared
# ------------------------------------------------------------

add_library(pluginshared INTERFACE)

target_include_directories(pluginshared
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/pluginshared/include
)

# ------------------------------------------------------------
# plugin
# ------------------------------------------------------------

if(DEFINED CI_PLUGIN_TARGET)
    add_subdirectory(${CI_PLUGIN_TARGET})
else()
    add_subdirectory(warpcore)
    add_subdirectory(empty)
    add_subdirectory(green_vocoder)
    add_subdirectory(steep_flanger)
    add_subdirectory(vital_reverb)
    add_subdirectory(vital_chorus)
    add_subdirectory(dispersive_delay)
    add_subdirectory(deep_phaser)
    add_subdirectory(resonator)
endif()
