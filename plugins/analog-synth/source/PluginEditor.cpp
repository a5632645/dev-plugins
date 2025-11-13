#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

AnalogSynthAudioProcessorEditor::AnalogSynthAudioProcessorEditor (AnalogSynthAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_(*p.preset_manager_)
    , osc1_(p.synth_)
    , osc2_(p.synth_)
    , osc3_(p.synth_)
    , osc4_(p.synth_)
    , noise_(p.synth_)
    , vol_env_(p.synth_, 1)
    , filter_env_(p.synth_, 2)
    , filter_(p.synth_)
    , lfo1_(p.synth_, 1)
    , lfo2_(p.synth_, 2)
    , lfo3_(p.synth_, 3)
    , modulations_(p)
    , fx_chain_(p.synth_, p)
    , marcos_(p.synth_)
    , voices_(p.synth_)
{
    addAndMakeVisible(preset_);

    addAndMakeVisible(osc1_);
    addAndMakeVisible(osc3_);
    addAndMakeVisible(osc2_);
    addAndMakeVisible(osc4_);
    addAndMakeVisible(vol_env_);
    addAndMakeVisible(filter_env_);
    addAndMakeVisible(filter_);
    addAndMakeVisible(lfo1_);
    addAndMakeVisible(lfo2_);
    addAndMakeVisible(lfo3_);
    addAndMakeVisible(modulations_);
    addAndMakeVisible(fx_chain_);
    addAndMakeVisible(noise_);
    addAndMakeVisible(marcos_);
    addAndMakeVisible(voices_);

    setSize(1150, 800 - 25);
}

AnalogSynthAudioProcessorEditor::~AnalogSynthAudioProcessorEditor() {
}

//==============================================================================
void AnalogSynthAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::green_bg);
}

void AnalogSynthAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    // top
    preset_.setBounds(b.removeFromTop(30));

    // left
    auto osc_bound = b.removeFromLeft(200);
    osc1_.setBounds(osc_bound.removeFromTop(110));
    osc2_.setBounds(osc_bound.removeFromTop(110));
    osc3_.setBounds(osc_bound.removeFromTop(215));
    osc4_.setBounds(osc_bound.removeFromTop(200));
    noise_.setBounds(osc_bound.removeFromTop(110));

    // most right
    auto fx_bound = b.removeFromRight(200);
    fx_chain_.setBounds(fx_bound);

    // right
    auto env_bound = b.removeFromRight(220);
    vol_env_.setBounds(env_bound.removeFromTop(90));
    filter_env_.setBounds(env_bound.removeFromTop(90));
    lfo1_.setBounds(env_bound.removeFromTop(90));
    lfo2_.setBounds(env_bound.removeFromTop(90));
    lfo3_.setBounds(env_bound.removeFromTop(90));
    marcos_.setBounds(env_bound.removeFromTop(90));
    voices_.setBounds(env_bound.removeFromTop(90));

    auto center_bound = b;
    // center top
    auto filter_bound = center_bound.removeFromTop(160);
    filter_.setBounds(filter_bound.removeFromLeft(200));
    // center down
    modulations_.setBounds(center_bound);
}
