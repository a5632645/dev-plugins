#pragma once

#include "ui/plugin_ui.hpp"
#include <pluginshared/plugin_editor.hpp>

class EmptyAudioProcessorEditor final
    : public pluginshared::PluginEditorBase<EmptyAudioProcessor, PluginUi> {

    using Base = pluginshared::PluginEditorBase<EmptyAudioProcessor, PluginUi>;
public:
    explicit EmptyAudioProcessorEditor(EmptyAudioProcessor& p)
        : Base(p) {}

    ~EmptyAudioProcessorEditor() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmptyAudioProcessorEditor)
};
