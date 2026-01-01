#include "PluginEditor.h"
#include "PluginProcessor.h"

EmptyAudioProcessorEditor::EmptyAudioProcessorEditor (EmptyAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_(*p.preset_manager_)
{
    addAndMakeVisible(preset_);

    auto& apvts = *p.value_tree_;

    setSize(400, 300);
}

EmptyAudioProcessorEditor::~EmptyAudioProcessorEditor() {
}

//==============================================================================
void EmptyAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::green_bg);

}

void EmptyAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(30));
}
