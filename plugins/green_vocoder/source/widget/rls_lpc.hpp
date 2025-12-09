#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace widget {

class RLSLPC : public juce::Component, private juce::Timer {
public:
    RLSLPC(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    void timerCallback() override;

    AudioPluginAudioProcessor& processor_;
    ui::Dial lpc_foorget_{"forget"};
    ui::FlatSlider lpc_dicimate_{"dicimate", ui::FlatSlider::TitleLayout::Top};
    ui::FlatSlider lpc_order_{"order", ui::FlatSlider::TitleLayout::Top};
    ui::Dial lpc_attack_{"attack"};
    ui::Dial lpc_release_{"release"};
};

}
