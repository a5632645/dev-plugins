#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::widget {

class Ensemble : public juce::Component {
public:
    Ensemble(AudioPluginAudioProcessor& p);
    void resized() override;
private:
    juce::Label title_{"", "Ensemble"};
    ui::FlatSlider num_voice_{"voices", ui::FlatSlider::TitleLayout::Top};
    ui::Dial detune_{"detune"};
    ui::Dial spread_{"spread"};
    ui::Dial mix_{"mix"};
    ui::Dial rate_{"rate"};
    ui::FlatCombobox mode_;
};

}
