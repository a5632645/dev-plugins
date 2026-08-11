#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::ui {

class STFTVocoder
    : public juce::Component
    , private juce::Timer {
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
    ::ui::Dial release_{"release"};
    ::ui::Dial attack_{"attack"};
    ::ui::FlatCombobox size_;
    ::ui::FlatCombobox mode_;

    ::ui::Dial blend_{"noisy"};
    ::ui::Dial bandwidth_{"smear"};
    ::ui::Dial detail_{"detail"};
    ::ui::Switch smooth_type_{"ERB", "OCT"};
    ::ui::Dial smooth_{"smooth"};
    ::ui::FlatSlider mfcc_size_;
    juce::Label mfcc_size_title_{"", "bands"};
};

} // namespace green_vocoder::ui
