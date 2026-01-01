#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

EmptyAudioProcessorEditor::EmptyAudioProcessorEditor (EmptyAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_(*p.preset_manager_)
{
    addAndMakeVisible(preset_);

    pitch_.BindParam(p.param_pitch_shift.ptr_);
    addAndMakeVisible(pitch_);

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

    auto box = b.removeFromTop(65);
    pitch_.setBounds(box.removeFromLeft(50));
}
