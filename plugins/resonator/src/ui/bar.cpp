#include "bar.hpp"
#include "../PluginProcessor.h"

BarGui::BarGui(ResonatorAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    midi_drive_.BindParam(apvts, "midi_drive");
    addAndMakeVisible(midi_drive_);
    round_robin_.BindParam(apvts, "round_robin");
    addAndMakeVisible(round_robin_);
    dry_.BindParam(apvts, "dry");
    addAndMakeVisible(dry_);
}

void BarGui::paint(juce::Graphics& g) {
    g.fillAll(ui::green_bg);
}

void BarGui::resized() {
    auto b = getLocalBounds();

    auto midi_b = b.removeFromLeft(80);
    midi_drive_.setBounds(midi_b.removeFromTop(midi_b.getHeight() / 2)
                        .reduced(2));
    round_robin_.setBounds(midi_b.reduced(2));

    dry_.setBounds(b.removeFromLeft(50));
}
