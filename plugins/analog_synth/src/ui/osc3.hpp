#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class Osc3Gui : public juce::Component {
public:
    Osc3Gui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_{"", "oscillator3"};
    ui::Dial detune_{"detune"};
    ui::Dial volume_{"volume"};
    ui::Dial pwm_{"pwm"};
    ui::CubeSelector waveform_;
    ui::FlatSlider unison_{"unison"};
    ui::FlatSlider unison_detune_{"detune"};
    ui::FlatCombobox unison_type_;
    ui::Switch retrigger_{"retrigger"};
    ui::FlatSlider phase_{"phase"};
    ui::FlatSlider random_{"random"};
};
}