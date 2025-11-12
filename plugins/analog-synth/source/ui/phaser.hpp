#pragma once
#include <pluginshared/component.hpp>
#include "bpm_dial.hpp"

namespace analogsynth {
class Synth;

class PhaserGui : public juce::Component {
public:
    PhaserGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    ui::Switch enable_{juce::String::fromUTF8("⭘")};
    juce::Label title_{"", "Phaser"};
    ui::Dial mix_{"mix"};
    ui::Dial center_{"center"};
    ui::Dial depth_{"depth"};
    BpmSyncDial rate_{"rate", "rate"};
    ui::Dial feedback_{"fb"};
    ui::Dial Q_{"Q"};
    ui::Dial stereo_{"stereo"};
};
}