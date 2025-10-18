#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

EmptyAudioProcessorEditor::EmptyAudioProcessorEditor (EmptyAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_(*p.preset_manager_)
{
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(preset_);

    setSize(800, 600);
}

EmptyAudioProcessorEditor::~EmptyAudioProcessorEditor() {
}

//==============================================================================
void EmptyAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::green_bg);

}

void EmptyAudioProcessorEditor::resized() {
    auto b = getLocalBounds();

    preset_.setBounds(b.removeFromTop(50));

}
