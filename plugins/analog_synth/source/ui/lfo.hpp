#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>
#include "bpm_dial.hpp"

namespace analogsynth {
class Synth;

class LfoGui : public juce::Component {
public:
    LfoGui(Synth& synth, size_t idx);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_;
    BpmSyncDial freq_{"freq", "tempo"};
    ui::Dial shape_{"shape"};
    ui::Switch retrigger_{"retrigger"};
    ui::FlatSlider phase_{"phase"};
};
}