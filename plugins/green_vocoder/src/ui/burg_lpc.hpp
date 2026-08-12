#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::ui {

class BurgLPC : public juce::Component {
public:
    BurgLPC(AudioPluginAudioProcessor& processor);
    void resized() override;

    void SetBlockMode(bool block_mode);
private:
    void MakeGui();

    AudioPluginAudioProcessor& processor_;
    bool block_mode_{false};
    ::ui::Dial forget_{"forget"};
    ::ui::Dial smear_{"smear"};
    ::ui::FlatSlider order_;
    juce::Label order_title_{"", "order"};
    ::ui::Dial attack_{"attack"};
    ::ui::Dial hold_{"hold"};
    ::ui::Dial release_{"release"};
    ::ui::FlatCombobox block_size_;
};

} // namespace green_vocoder::ui
