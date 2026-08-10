#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::ui {
class Tracking : public juce::Component {
public:
    Tracking(AudioPluginAudioProcessor& p);

    void resized() override;
private:
    juce::Label title_{"tracking", "tracking"};
    ::ui::FlatSlider fmin_;
    juce::Label fmin_title_{"", "fmin"};
    ::ui::FlatSlider fmax_;
    juce::Label fmax_title_{"", "fmax"};
    ::ui::Dial pitch_{"pitch"};
    ::ui::Dial pwm_{"pwm"};
    ::ui::FlatCombobox waveform_;
    ::ui::FlatSlider noise_;
    juce::Label noise_title_{"", "noise"};
    ::ui::FlatSlider glide_;
    juce::Label glide_title_{"", "glide"};
};
} // namespace green_vocoder::ui
