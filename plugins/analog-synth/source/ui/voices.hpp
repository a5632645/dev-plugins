#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class VoicesGui : public juce::Component {
public:
    VoicesGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_{"", "Voices"};
    ui::Switch legato_{"legato"};
    ui::Switch porta_{"porta"};
    ui::Dial time_{"time"};
    ui::Switch mono_{"mono"};
    ui::FlatSlider voice_{"voice"};
};
}