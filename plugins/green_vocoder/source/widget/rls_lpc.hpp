#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "ui/vertical_slider.hpp"

class AudioPluginAudioProcessor;

namespace widget {

class RLSLPC : public juce::Component, private juce::Timer {
public:
    RLSLPC(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void OnLanguageChanged(tooltip::Tooltips& tooltips);
private:
    void timerCallback() override;

    AudioPluginAudioProcessor& processor_;
    juce::Label lpc_label_;
    ui::VerticalSlider lpc_foorget_;
    ui::VerticalSlider lpc_dicimate_;
    ui::VerticalSlider lpc_order_;
    ui::VerticalSlider lpc_attack_;
    ui::VerticalSlider lpc_release_;
};

}