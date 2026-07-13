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
    void OnFilterTypeChanged();

    ui::Switch enable_{juce::String::fromUTF8("⭘")};
    juce::Label title_{"", "Filter"};
    ui::FlatCombobox type_;
    ui::Dial cutoff_{"cutoff"};
    ui::Dial resonance_{"reso"};
    ui::Dial morph_{"morph"};
    ui::Dial mix_{"mix"};
    ui::Dial var1_{"var1"};
    ui::Dial var2_{"var2"};
    ui::Dial var3_{"var3"};
};
}