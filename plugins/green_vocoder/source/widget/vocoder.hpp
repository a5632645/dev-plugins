#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::widget {
class Vocoder : public juce::Component, private juce::Timer, private juce::ComboBox::Listener {
public:
    Vocoder(AudioPluginAudioProcessor& p);
    ~Vocoder() override;
    void resized() override;
private:
    void comboBoxChanged(juce::ComboBox* box) override;
    void timerCallback() override;

    juce::Label title_{"", "Vocoder"};
    ui::FlatSlider shift_pitch_{"formant"};
    ui::FlatCombobox vocoder_type_;

    juce::Component* current_vocoder_widget_{};
    std::unique_ptr<juce::Component> burg_;
    std::unique_ptr<juce::Component> channel_;
    std::unique_ptr<juce::Component> stft_;
    std::unique_ptr<juce::Component> mfcc_;
};
}
