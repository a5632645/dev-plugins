#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "ui/vertical_slider.hpp"

class AudioPluginAudioProcessor;

namespace widget {

class BurgLPC : public juce::Component, private juce::Timer {
public:
    BurgLPC(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    void timerCallback() override;

    AudioPluginAudioProcessor& processor_;
    juce::Label lpc_label_;
    ui::VerticalSlider lpc_learn_;
    ui::VerticalSlider lpc_foorget_;
    ui::VerticalSlider lpc_smooth_;
    ui::VerticalSlider lpc_dicimate_;
    ui::VerticalSlider lpc_order_;
    ui::VerticalSlider lpc_attack_;
    ui::VerticalSlider lpc_release_;
};

}