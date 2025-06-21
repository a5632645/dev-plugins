#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "ui/vertical_slider.hpp"

class AudioPluginAudioProcessor;

namespace widget {

class CepstrumVocoderUI : public juce::Component, private juce::Timer {
public:
    CepstrumVocoderUI(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    void timerCallback() override;

    AudioPluginAudioProcessor& processor_;
    juce::Label title_;
    ui::VerticalSlider omega_;
    ui::VerticalSlider release_;
};

}