#pragma once
#include <pluginshared/component.hpp>
#include "dsp/channel_vocoder.hpp"

class AudioPluginAudioProcessor;

namespace green_vocoder::widget {

class ChannelVocoder : public juce::Component {
public:
    ChannelVocoder(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    dsp::ChannelVocoder& vocoder_;
    ui::Dial attack_{"attack"};
    ui::Dial release_{"release"};
    ui::FlatSlider nbands_{"nbands", ui::FlatSlider::TitleLayout::Top};
    ui::FlatSlider freq_begin_{"freq_begin", ui::FlatSlider::TitleLayout::Top};
    ui::FlatSlider freq_end_{"freq_end", ui::FlatSlider::TitleLayout::Top};
    ui::Dial scale_{"scale"};
    ui::Dial carry_scale_{"carry bw"};
    ui::FlatCombobox map_;
    juce::Label label_filter_bank_{"", "filter bank mode"};
    ui::FlatCombobox filter_bank_;
    ui::Dial gate_{"gate"};
};

}
