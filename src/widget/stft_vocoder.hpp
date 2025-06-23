#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "ui/vertical_slider.hpp"

class AudioPluginAudioProcessor;

namespace widget {

class STFTVocoder : public juce::Component, private juce::Timer {
public:
    STFTVocoder(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    void timerCallback() override;

    AudioPluginAudioProcessor& processor_;
    juce::Label title_;
    ui::VerticalSlider bandwidth_;
    ui::VerticalSlider release_;
    ui::VerticalSlider attack_;
    ui::VerticalSlider blend_;
};

}