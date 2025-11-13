#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class FilterGui : public juce::Component {
public:
    FilterGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    ui::Switch enable_{juce::String::fromUTF8("⭘")};
    juce::Label title_{"", "Filter"};
    ui::Dial cutoff_{"cutoff"};
    ui::Dial Q_{"Q"};
    ui::Dial direct_{"direct"};
    ui::Dial lp_{"lp"};
    ui::Dial hp_{"hp"};
    ui::Dial bp_{"bp"};
};
}