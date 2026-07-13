#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class Osc2Gui : public juce::Component {
public:
    Osc2Gui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_{"", "Oscillator2"};
    ui::Dial detune_{"detune"};
    ui::Dial volume_{"volume"};
    ui::Dial pwm_{"pwm"};
    ui::CubeSelector waveform_;
    ui::Switch sync_{"s"};
};
}