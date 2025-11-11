#include <pluginshared/component.hpp>
#include "bpm_dial.hpp"

namespace analogsynth {
class Synth;

class ChorusGui : public juce::Component {
public:
    ChorusGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    ui::Switch enable_{juce::String::fromUTF8("⭘")};
    juce::Label title_{"", "Chorus"};
    ui::Dial delay_{"delay"};
    BpmSyncDial rate_{"rate", "rate"};
    ui::Dial feedback_{"feedback"};
    ui::Dial mix_{"mix"};
    ui::Dial depth_{"depth"};
};
}