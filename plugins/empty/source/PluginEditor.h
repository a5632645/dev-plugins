#pragma once
#include "pluginshared/component.hpp"
#include "pluginshared/preset_panel.hpp"

// ---------------------------------------- editor ----------------------------------------

class EmptyAudioProcessor;

//==============================================================================
class EmptyAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit EmptyAudioProcessorEditor (EmptyAudioProcessor&);
    ~EmptyAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EmptyAudioProcessor& p_;

    pluginshared::PresetPanel preset_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EmptyAudioProcessorEditor)
};
