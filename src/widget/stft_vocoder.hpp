#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include "tooltips.hpp"
#include "ui/vertical_slider.hpp"

class AudioPluginAudioProcessor;

namespace widget {

class STFTVocoder : public juce::Component, private juce::Timer, public tooltip::Tooltips::Listener {
public:
    STFTVocoder(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void OnLanguageChanged(tooltip::Tooltips& tooltips) override;
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