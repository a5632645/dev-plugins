#pragma once
#include "../../shared/component.hpp"

// ---------------------------------------- editor ----------------------------------------

class SimpleReverbAudioProcessor;

//==============================================================================
class SimpleReverbAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit SimpleReverbAudioProcessorEditor (SimpleReverbAudioProcessor&);
    ~SimpleReverbAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SimpleReverbAudioProcessor& p_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleReverbAudioProcessorEditor)
};
