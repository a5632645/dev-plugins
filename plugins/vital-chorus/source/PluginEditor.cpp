#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

VitalChorusAudioProcessorEditor::VitalChorusAudioProcessorEditor (VitalChorusAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
{
    auto& apvts = *p.value_tree_;
}

VitalChorusAudioProcessorEditor::~VitalChorusAudioProcessorEditor() {
}

//==============================================================================
void VitalChorusAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colour{22,27,32});

}

void VitalChorusAudioProcessorEditor::resized() {
    auto b = getLocalBounds();

}
