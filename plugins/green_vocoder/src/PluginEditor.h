#pragma once

#include <pluginshared/plugin_editor.hpp>
#include "ui/plugin_ui.hpp"

class AudioPluginAudioProcessorEditor final
    : public pluginshared::PluginEditorBase<AudioPluginAudioProcessor, green_vocoder::ui::PluginUi> {
    using Base = pluginshared::PluginEditorBase<AudioPluginAudioProcessor, green_vocoder::ui::PluginUi>;
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
        : Base(p) {}

    ~AudioPluginAudioProcessorEditor() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
