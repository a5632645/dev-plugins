#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class AdsrGui : public juce::Component {
public:
    AdsrGui(Synth& synth, size_t idx);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_;
    ui::Dial attack_{"attack"};
    ui::Dial decay_{"decay"};
    ui::Dial sustain_{"sustain"};
    ui::Dial release_{"release"};
    ui::Switch exp_{"exp", "lin"};
};
}