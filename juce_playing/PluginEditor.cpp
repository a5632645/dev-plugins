#include "PluginProcessor.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "param_ids.hpp"
#include "ui/vertical_slider.hpp"
#include "ui/xypad.hpp"
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

    w0_.BindParameter(apvsts, "w0");
    w0_.SetShortName("w0");
    addAndMakeVisible(w0_);

    w_.BindParameter(apvsts, "w");
    w_.SetShortName("w");
    addAndMakeVisible(w_);

    n_.BindParameter(apvsts, "n");
    n_.SetShortName("n");
    addAndMakeVisible(n_);

    g_.BindParameter(apvsts, "gain");
    g_.SetShortName("gain");
    addAndMakeVisible(g_);

    pad_.SetMode(ui::XYPad::Mode::XY);
    pad_.BindParamX(apvsts, "m");
    pad_.BindParamY(apvsts, "p");
    addAndMakeVisible(pad_);

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
    w0_.setBounds(b.removeFromLeft(50));
    w_.setBounds(b.removeFromLeft(50));
    n_.setBounds(b.removeFromLeft(50));
    g_.setBounds(b.removeFromLeft(50));
    pad_.setBounds(b.removeFromLeft(b.getHeight()));
    pad_.EvalCirclePos();
}

void AudioPluginAudioProcessorEditor::timerCallback() {
}

void AudioPluginAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) {
    
}
