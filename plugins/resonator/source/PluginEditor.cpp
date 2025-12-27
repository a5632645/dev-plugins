#include "PluginEditor.h"
#include "PluginProcessor.h"

ResonatorAudioProcessorEditor::ResonatorAudioProcessorEditor (ResonatorAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_panel_(*p.preset_manager_)
    , bar_(p)
    , couple_(p)
    , resonators_(p)
{
    addAndMakeVisible(preset_panel_);
    addAndMakeVisible(bar_);
    addAndMakeVisible(couple_);
    addAndMakeVisible(resonators_);

    bar_.GetMidiDrive().onClick = [this] {
        resonators_.SetMidiDrive(bar_.GetMidiDrive().getToggleState());
    };

    setSize(640, 600);
}

ResonatorAudioProcessorEditor::~ResonatorAudioProcessorEditor() {
}

//==============================================================================
void ResonatorAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::black_bg);
}

void ResonatorAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    preset_panel_.setBounds(b.removeFromTop(30));
    resonators_.setBounds(b.removeFromLeft(480));
    auto side_b = b;
    bar_.setBounds(side_b.removeFromTop(65).reduced(2));
    couple_.setBounds(side_b.reduced(2));
}
