#pragma once

#include "PluginProcessor.h"
#include <array>
#include "ui/vertical_slider.hpp"
#include "ui/toggle_button.hpp"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    AudioPluginAudioProcessor& processorRef;

    ui::VerticalSlider em_pitch_;
    ui::VerticalSlider em_gain_;
    ui::VerticalSlider em_s_;
    ui::VerticalSlider hp_pitch_;
    ui::VerticalSlider shift_pitch_;
    ui::VerticalSlider lpc_learn_;
    ui::VerticalSlider lpc_foorget_;
    ui::VerticalSlider lpc_smooth_;
    ui::VerticalSlider lpc_order_;
    ui::VerticalSlider lpc_attack_;
    ui::VerticalSlider lpc_release_;
    juce::Label filter_{"", "filter"};
    juce::Label shifter_{"", "shifter"};
    juce::Label lpc_{"", "lpc"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
