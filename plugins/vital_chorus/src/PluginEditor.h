#pragma once

#include "ui/plugin_ui.hpp"
#include <pluginshared/plugin_editor.hpp>

class VitalChorusAudioProcessorEditor final
    : public pluginshared::PluginEditorBase<VitalChorusAudioProcessor, PluginUi> {

    using Base = pluginshared::PluginEditorBase<VitalChorusAudioProcessor, PluginUi>;
public:
    explicit VitalChorusAudioProcessorEditor(VitalChorusAudioProcessor& p)
        : Base(p) {}

    ~VitalChorusAudioProcessorEditor() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VitalChorusAudioProcessorEditor)
};
