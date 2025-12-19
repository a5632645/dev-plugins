#pragma once
#include "pluginshared/component.hpp"
#include "pluginshared/preset_panel.hpp"

// ---------------------------------------- editor ----------------------------------------

class EwarpAudioProcessor;

//==============================================================================
class EwarpAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit EwarpAudioProcessorEditor (EwarpAudioProcessor&);
    ~EwarpAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EwarpAudioProcessor& p_;

    pluginshared::PresetPanel preset_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EwarpAudioProcessorEditor)
};
