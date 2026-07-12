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
    void DrawStandardCepstrum(juce::Graphics& g);
    void DrawMfcc(juce::Graphics& g);
    void OnModeChanged();

    AudioPluginAudioProcessor& processor_;
    ui::Dial release_{"release"};
    ui::Dial attack_{"attack"};
    ui::FlatCombobox size_;
    ui::FlatSelector mode_;
    
    ui::Dial blend_{"noisy"};
    ui::Dial bandwidth_{"smear"};
    ui::Dial detail_{"detail"};
    ui::FlatSlider mfcc_size_{"bands", ui::FlatSlider::TitleLayout::Top};
};

}
