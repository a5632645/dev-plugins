#include "PluginProcessor.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "param_ids.hpp"
#include "ui/vertical_slider.hpp"
#include <cstddef>
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
    , tooltip_window_(this, 500)
{
    setLookAndFeel(&look_);
    tooltip_window_.setLookAndFeel(&look_);
    auto& apvsts = *p.value_tree_;

    shift_.BindParameter(apvsts, id::kShift);
    shift_.SetShortName("shift");
    addAndMakeVisible(shift_);

    setSize (480, 480);
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
    shift_.setBounds(b.removeFromLeft(50));
}

void AudioPluginAudioProcessorEditor::timerCallback() {
}

void AudioPluginAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) {
    
}
