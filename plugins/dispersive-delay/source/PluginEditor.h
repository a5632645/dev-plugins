#pragma once
#include "../../shared/component.hpp"

#include "ui/LM_slider.h"
#include "ui/common_curve_editor.h"

// ---------------------------------------- editor ----------------------------------------

class DispersiveDelayAudioProcessor;

//==============================================================================
class DispersiveDelayAudioProcessorEditor final 
    : public juce::AudioProcessorEditor
    , private juce::Timer {
public:
    explicit DispersiveDelayAudioProcessorEditor (DispersiveDelayAudioProcessor&);
    ~DispersiveDelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

private:
    void timerCallback() override;

    DispersiveDelayAudioProcessor& p_;

    LMKnob beta_;
    LMKnob f_begin_;
    LMKnob f_end_;
    LMKnob delay_time_;
    LMKnob min_bw_;
    mana::CommonCurveEditor curve_;
    juce::Label num_filter_label_;
    juce::ToggleButton x_axis_;
    std::unique_ptr<juce::ButtonParameterAttachment> x_axis_attachment_;

    juce::Label res_label_;
    juce::ComboBox reslution_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> resolution_attachment_;

    juce::TextButton random_;
    juce::TextButton clear_curve_;
    juce::TextButton panic_;

    std::vector<float> group_delay_cache_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DispersiveDelayAudioProcessorEditor)
};
