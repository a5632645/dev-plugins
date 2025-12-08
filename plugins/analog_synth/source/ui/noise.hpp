#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class NoiseGui : public juce::Component {
public:
    NoiseGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_{"", "Noise"};
    ui::Dial volume_{"volume"};
    ui::CubeSelector type_;
};
}