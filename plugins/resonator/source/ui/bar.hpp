#pragma once
#include <pluginshared/component.hpp>

class ResonatorAudioProcessor;

class BarGui : public juce::Component {
public:
    BarGui(ResonatorAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;

    ui::Switch& GetMidiDrive() { return midi_drive_; }
private:
    ui::Switch midi_drive_{"midi"};
    ui::Switch round_robin_{"round_robin"};
    ui::Dial dry_{"dry"};
};
