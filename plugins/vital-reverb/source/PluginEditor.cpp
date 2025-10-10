#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

SimpleReverbAudioProcessorEditor::SimpleReverbAudioProcessorEditor (SimpleReverbAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
{
    auto& apvts = *p.value_tree_;
}

SimpleReverbAudioProcessorEditor::~SimpleReverbAudioProcessorEditor() {
}

//==============================================================================
void SimpleReverbAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colour{22,27,32});

}

void SimpleReverbAudioProcessorEditor::resized() {
    auto b = getLocalBounds();

}
