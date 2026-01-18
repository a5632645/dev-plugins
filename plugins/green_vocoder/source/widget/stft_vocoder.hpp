#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::widget {

class STFTVocoder : public juce::Component, private juce::Timer {
public:
    STFTVocoder(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    void timerCallback() override;

    AudioPluginAudioProcessor& processor_;
    ui::Dial bandwidth_{"smear"};
    ui::Dial release_{"release"};
    ui::Dial attack_{"attack"};
    ui::Dial blend_{"noisy"};
    ui::FlatCombobox size_;

    ui::Switch use_v2_{"v2", "v1"};
    ui::Dial detail_{"detail"};
};

}
