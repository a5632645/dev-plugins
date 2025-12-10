#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::widget {

class MFCCVocoder : public juce::Component {
public:
    MFCCVocoder(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    AudioPluginAudioProcessor& p_;
    ui::Dial attack_{"attack"};
    ui::Dial release_{"release"};
    ui::FlatCombobox fft_size_;
    ui::FlatSlider mfcc_size_{"band", ui::FlatSlider::TitleLayout::Left};
};

}
