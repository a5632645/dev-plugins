#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

EwarpAudioProcessorEditor::EwarpAudioProcessorEditor (EwarpAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_(*p.preset_manager_)
{
    addAndMakeVisible(preset_);

    warp_.BindParam(p.warp_bands_.ptr_);
    addAndMakeVisible(warp_);

    ratio_.BindParam(p.ratio_.ptr_);
    addAndMakeVisible(ratio_);

    am2rm_.BindParam(p.am2rm_.ptr_);
    addAndMakeVisible(am2rm_);
    
    decay_.BindParam(p.decay_.ptr_);
    addAndMakeVisible(decay_);

    reverse_.BindParam(p.reverse_.ptr_);
    addAndMakeVisible(reverse_);

    setSize(400, 160);
}

EwarpAudioProcessorEditor::~EwarpAudioProcessorEditor() {
}

//==============================================================================
void EwarpAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::green_bg);
}

void EwarpAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(30));

    auto w = b.getWidth() / 5;
    warp_.setBounds(b.removeFromLeft(w).reduced(2));
    ratio_.setBounds(b.removeFromLeft(w).reduced(2));
    am2rm_.setBounds(b.removeFromLeft(w).reduced(2));
    decay_.setBounds(b.removeFromLeft(w).reduced(2));
    reverse_.setBounds(b.reduced(2));
}
