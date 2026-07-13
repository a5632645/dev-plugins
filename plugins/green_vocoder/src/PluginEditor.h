#pragma once

#include "ui/plugin_ui.hpp"
#include <pluginshared/plugin_editor.hpp>

class AudioPluginAudioProcessorEditor final
    : public pluginshared::PluginEditorBase<AudioPluginAudioProcessor, PluginUi> {

    using Base = pluginshared::PluginEditorBase<AudioPluginAudioProcessor, PluginUi>;
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
        : Base(p) {}

    ~AudioPluginAudioProcessorEditor() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
