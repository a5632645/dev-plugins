#pragma once
#include <pluginshared/component.hpp>
#include "dsp/channel_vocoder.hpp"

class AudioPluginAudioProcessor;

namespace green_vocoder::ui {

class ChannelVocoder : public juce::Component {
public:
    ChannelVocoder(AudioPluginAudioProcessor& processor);
    void resized() override;
private:
    dsp::ChannelVocoder& vocoder_;
    ::ui::Dial attack_{"attack"};
    ::ui::Dial release_{"release"};
    ::ui::FlatSlider nbands_;
    juce::Label nbands_title_{"", "nbands"};
    ::ui::FlatSlider freq_begin_;
    juce::Label freq_begin_title_{"", "freq_begin"};
    ::ui::FlatSlider freq_end_;
    juce::Label freq_end_title_{"", "freq_end"};
    ::ui::Dial scale_{"scale"};
    ::ui::Dial carry_scale_{"carry bw"};
    ::ui::FlatCombobox map_;
    juce::Label label_filter_bank_{"", "filter bank mode"};
    ::ui::FlatCombobox filter_bank_;
    ::ui::FlatSlider ripple_;
    juce::Label ripple_title_{"", "ripple"};
    ::ui::Dial gate_{"gate"};
};

} // namespace green_vocoder::ui
