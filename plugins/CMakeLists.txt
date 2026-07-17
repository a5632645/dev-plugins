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

# VST3_AUTO_MANIFEST: CI 单插件构建时可安全启用 manifest 生成
# 本地多插件构建时禁用以避免 shared_incs_*.txt 冲突
if(DEFINED CI_PLUGIN_TARGET)
    set(PLUGIN_VST3_AUTO_MANIFEST TRUE)
    add_subdirectory(${CI_PLUGIN_TARGET})
else()
    set(PLUGIN_VST3_AUTO_MANIFEST FALSE)
    add_subdirectory(warpcore)
    add_subdirectory(empty)
    add_subdirectory(green_vocoder)
    add_subdirectory(steep_flanger)
    add_subdirectory(vital_reverb)
    add_subdirectory(vital_chorus)
    add_subdirectory(dispersive_delay)
    add_subdirectory(deep_phaser)
    add_subdirectory(resonator)
    add_subdirectory(sttr)
endif()
