#include "PluginProcessor.h"
#include "dsp/lattice_apf.hpp"
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

    reflection_.BindParameter(apvsts, id::kK);
    reflection_.SetShortName("ref");
    reflection_.slider_.setTooltip("internal feedback in lattice allpass filter");
    addAndMakeVisible(reflection_);
    begin_.BindParameter(apvsts, id::kBegin);
    begin_.SetShortName("begin");
    begin_.slider_.setTooltip("begin delay time");
    addAndMakeVisible(begin_);
    end_.BindParameter(apvsts, id::kEnd);
    end_.SetShortName("end");
    end_.slider_.setTooltip("end delay time");
    addAndMakeVisible(end_);
    freq_.BindParameter(apvsts, id::kFreq);
    freq_.SetShortName("freq");
    freq_.slider_.setTooltip("noise frequency");
    addAndMakeVisible(freq_);
    num_.BindParameter(apvsts, id::kNum);
    num_.SetShortName("num");
    num_.slider_.setTooltip("num of nested allpass");
    addAndMakeVisible(num_);
    mix_.BindParameter(apvsts, id::kMix);
    mix_.SetShortName("mix");
    mix_.slider_.setTooltip("dry wet");
    addAndMakeVisible(mix_);
    mono_.BindParameter(apvsts, id::kMono);
    mono_.setTooltip("use mono modulator");
    mono_.setButtonText("mono");
    addAndMakeVisible(mono_);
    altk_.BindParameter(apvsts, id::kAltK);
    altk_.setTooltip("pos neg switch inner reflection");
    altk_.setButtonText("alt");
    addAndMakeVisible(altk_);
    text_.setText("please move ref slowly", juce::dontSendNotification);
    addAndMakeVisible(text_);

    setSize (300, 130);
    // startTimerHz(30);
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
    auto top = b.removeFromTop(30);
    altk_.setBounds(top.removeFromRight(60));
    mono_.setBounds(top.removeFromRight(60));
    text_.setBounds(top);
    reflection_.setBounds(b.removeFromLeft(50));
    begin_.setBounds(b.removeFromLeft(50));
    end_.setBounds(b.removeFromLeft(50));
    freq_.setBounds(b.removeFromLeft(50));
    num_.setBounds(b.removeFromLeft(50));
    mix_.setBounds(b.removeFromLeft(50));
}

void AudioPluginAudioProcessorEditor::timerCallback() {
}

void AudioPluginAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) {
    
}
