#pragma once

#include <pluginshared/plugin_editor.hpp>
#include "ui/plugin_ui.hpp"

class SttrAudioProcessorEditor final : public pluginshared::PluginEditorBase<SttrAudioProcessor, PluginUi> {
    using Base = pluginshared::PluginEditorBase<SttrAudioProcessor, PluginUi>;
public:
    explicit SttrAudioProcessorEditor(SttrAudioProcessor& p)
        : Base(p) {}

    ~SttrAudioProcessorEditor() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SttrAudioProcessorEditor)
};
