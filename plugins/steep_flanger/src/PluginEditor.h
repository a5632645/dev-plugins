#pragma once

#include "ui/plugin_ui.hpp"
#include <pluginshared/plugin_editor.hpp>

class EmptyAudioProcessorEditor final
    : public pluginshared::PluginEditorBase<SteepFlangerAudioProcessor, PluginUi> {

    using Base = pluginshared::PluginEditorBase<SteepFlangerAudioProcessor, PluginUi>;
public:
    explicit EmptyAudioProcessorEditor(SteepFlangerAudioProcessor& p)
        : Base(p) {}

    ~EmptyAudioProcessorEditor() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmptyAudioProcessorEditor)
};
