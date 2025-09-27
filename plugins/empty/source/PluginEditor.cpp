#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

SteepFlangerAudioProcessorEditor::SteepFlangerAudioProcessorEditor (SteepFlangerAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
{
    auto& apvts = *p.value_tree_;
}

SteepFlangerAudioProcessorEditor::~SteepFlangerAudioProcessorEditor() {
}

//==============================================================================
void SteepFlangerAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colour{22,27,32});

}

void SteepFlangerAudioProcessorEditor::resized() {
    auto b = getLocalBounds();

}
