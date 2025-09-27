#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// ---------------------------------------- editor ----------------------------------------

class SteepFlangerAudioProcessor;

//==============================================================================
class SteepFlangerAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit SteepFlangerAudioProcessorEditor (SteepFlangerAudioProcessor&);
    ~SteepFlangerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SteepFlangerAudioProcessor& p_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessorEditor)
};
