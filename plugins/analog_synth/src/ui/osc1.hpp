#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class Osc1Gui : public juce::Component {
public:
    Osc1Gui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_{"", "Oscillator1"};
    ui::Dial detune_{"detune"};
    ui::Dial volume_{"volume"};
    ui::Dial pwm_{"pwm"};
    ui::CubeSelector waveform_;
};
}