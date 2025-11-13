#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class MarcosGui : public juce::Component {
public:
    MarcosGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_{"", "Marco"};
    ui::Dial marco1_{"1"};
    ui::Dial marco2_{"2"};
    ui::Dial marco3_{"3"};
    ui::Dial marco4_{"4"};
};
}