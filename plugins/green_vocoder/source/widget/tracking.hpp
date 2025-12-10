#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::widget {
class Tracking : public juce::Component {
public:
    Tracking(AudioPluginAudioProcessor& p);

    void resized() override;
private:
    juce::Label title_{"tracking", "tracking"};
    ui::FlatSlider fmin_{"fmin", ui::FlatSlider::TitleLayout::Top};
    ui::FlatSlider fmax_{"fmax", ui::FlatSlider::TitleLayout::Top};
    ui::Dial pitch_{"pitch"};
    ui::Dial pwm_{"pwm"};
    ui::FlatCombobox waveform_;
    ui::FlatSlider noise_{"noise"};
    ui::FlatSlider glide_{"glide"};
};
}
