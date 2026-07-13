#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::widget {
class PreFx : public juce::Component {
public:
    PreFx(AudioPluginAudioProcessor& p);
    void resized() override;
private:
    juce::Label title_{"", "pre fx"};
    ui::Dial tilt_{"tilt"};
    ui::FlatCombobox main_route_;
    ui::FlatCombobox side_route_;
    juce::Label main_route_title_{"", "modulator"};
    juce::Label side_route_title_{"", "carry"};
};
}
