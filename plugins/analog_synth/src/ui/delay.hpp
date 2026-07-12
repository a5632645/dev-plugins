#pragma once
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class DelayGui : public juce::Component {
public:
    DelayGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    ui::Switch enable_{juce::String::fromUTF8("⭘")};
    juce::Label title_{"", "Delay"};
    ui::Switch pingpong_{"pingpong", "mono"};
    ui::Dial delay_{"delay"};
    ui::Dial lowcut_{"lowcut"};
    ui::Dial highcut_{"highcut"};
    ui::Dial feedback_{"feedback"};
    ui::Dial mix_{"mix"};
};
}