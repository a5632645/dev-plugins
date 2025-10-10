#pragma once
#include "../../shared/component.hpp"

// ---------------------------------------- editor ----------------------------------------

class VitalChorusAudioProcessor;

//==============================================================================
class VitalChorusAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit VitalChorusAudioProcessorEditor (VitalChorusAudioProcessor&);
    ~VitalChorusAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VitalChorusAudioProcessor& p_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VitalChorusAudioProcessorEditor)
};
