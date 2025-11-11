#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class DistortionGui : public juce::Component {
public:
    DistortionGui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    ui::Switch enable_{juce::String::fromUTF8("⭘")};
    juce::Label title_{"", "Distortion"};
    ui::Dial drive_{"drive"};
};
}