#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::ui {
class PreFx : public juce::Component {
public:
    PreFx(AudioPluginAudioProcessor& p);
    void resized() override;
private:
    juce::Label title_{"", "pre fx"};
    ::ui::Dial tilt_{"tilt"};
    ::ui::Switch swap_{"swap"};
    ::ui::FlatCombobox pitch_ch_;
    juce::Label swap_title_{"", "ch swap"};
    juce::Label pitch_title_{"", "pitch ch"};
};
} // namespace green_vocoder::ui
