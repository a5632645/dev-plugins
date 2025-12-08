#pragma once
#include <pluginshared/component.hpp>
#include "bpm_dial.hpp"

namespace analogsynth {
class Synth;

class ReverbGui : public juce::Component {
public:
    ReverbGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    ui::Switch enable_{juce::String::fromUTF8("⭘")};
    juce::Label title_{"", "Reverb"};
    ui::Dial mix_{"mix"};
    ui::Dial predelay_{"predelay"};
    ui::Dial lowpass_{"lowpass"};
    ui::Dial decay_{"decay"};
    ui::Dial size_{"size"};
    ui::Dial damp_{"damp"};
};
}