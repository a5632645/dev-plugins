#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

EwarpAudioProcessorEditor::EwarpAudioProcessorEditor (EwarpAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_(*p.preset_manager_)
{
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(preset_);

    setSize(800, 600);
}

EwarpAudioProcessorEditor::~EwarpAudioProcessorEditor() {
}

//==============================================================================
void EwarpAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::green_bg);

}

void EwarpAudioProcessorEditor::resized() {
    auto b = getLocalBounds();

    preset_.setBounds(b.removeFromTop(50));

}
