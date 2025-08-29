#include "PluginEditor.h"

#include <cstddef>

#include "PluginProcessor.h"
#include "juce_graphics/juce_graphics.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
    , tooltip_window_(this, 500)
{
    setLookAndFeel(&look_);
    tooltip_window_.setLookAndFeel(&look_);
    auto& apvsts = *p.value_tree_;

    setSize (400, 200);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
    tooltip_window_.setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    
}

void AudioPluginAudioProcessorEditor::timerCallback() {
}

