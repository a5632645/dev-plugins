#pragma once
#include "plugin_ui.hpp"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void resized() override;
private:
    PluginUi ui_;

    struct PluginConfig;
    juce::SharedResourcePointer<PluginConfig> plugin_config_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
