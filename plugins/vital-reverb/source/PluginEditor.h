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

    ui::Dial chorus_amount_{"CHOR AMT"};
    ui::Dial chorus_freq_{"CHOR FREQ"};
    ui::Dial mix_{"MIX"};
    ui::Dial pre_lowpass_{"LOW CUT"};
    ui::Dial pre_highpass_{"HIGH CUT"};
    ui::Dial low_damp_{"LOW DAMP"};
    ui::Dial high_damp_{"HIGH DAMP"};
    ui::Dial low_gain_{"LOW GAIN"};
    ui::Dial high_gain_{"HIGH GAIN"};
    ui::Dial size_{"SIZE"};
    ui::Dial decay_{"TIME"};
    ui::Dial predelay_{"DELAY"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleReverbAudioProcessorEditor)
};
