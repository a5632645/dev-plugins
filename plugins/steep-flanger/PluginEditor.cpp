#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
SteepFlangerAudioProcessorEditor::SteepFlangerAudioProcessorEditor (SteepFlangerAudioProcessor& p)
    : AudioProcessorEditor (&p)
{
    setSize (575, 550);
}

SteepFlangerAudioProcessorEditor::~SteepFlangerAudioProcessorEditor() {
}

//==============================================================================
void SteepFlangerAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void SteepFlangerAudioProcessorEditor::resized() {
}
