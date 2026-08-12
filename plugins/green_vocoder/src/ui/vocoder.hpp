#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::ui {
class Vocoder : public juce::Component {
public:
    Vocoder(AudioPluginAudioProcessor& p);
    ~Vocoder() override;
    void resized() override;
private:
    void OnVocoderTypeChanged();

    juce::Label title_{"", "Vocoder"};
    ::ui::FlatSlider shift_pitch_;
    juce::Label shift_pitch_title_{"", "formant"};
    ::ui::FlatSelector vocoder_type_;

    juce::Component* current_vocoder_widget_{};
    std::unique_ptr<juce::Component> burg_;
    std::unique_ptr<juce::Component> channel_;
    std::unique_ptr<juce::Component> stft_;
};
} // namespace green_vocoder::ui
