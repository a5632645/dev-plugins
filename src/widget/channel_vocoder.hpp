#pragma once
#include "dsp/channel_vocoder.hpp"
#include "juce_gui_basics/juce_gui_basics.h"
#include "ui/vertical_slider.hpp"

class AudioPluginAudioProcessor;

namespace widget {

class ChannelVocoder : public juce::Component {
public:
    ChannelVocoder(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    dsp::ChannelVocoder& vocoder_;
    juce::Label label_;
    ui::VerticalSlider attack_;
    ui::VerticalSlider release_;
    ui::VerticalSlider nbands_;
    ui::VerticalSlider freq_begin_;
    ui::VerticalSlider freq_end_;
    ui::VerticalSlider q_;
};

}