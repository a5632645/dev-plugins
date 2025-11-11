#include "lfo.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
LfoGui::LfoGui(Synth& synth, size_t idx) {
    switch (idx) {
        case 1:
            title_.setText("LFO1", juce::dontSendNotification);
            freq_.BindParam(synth.param_lfo1_freq.state_);
            shape_.BindParam(synth.param_lfo1_shape.ptr_);
            retrigger_.BindParam(synth.param_lfo1_retrigger.ptr_);
            phase_.BindParam(synth.param_lfo1_phase.ptr_);
            break;
        case 2:
            title_.setText("LFO2", juce::dontSendNotification);
            freq_.BindParam(synth.param_lfo2_freq.state_);
            shape_.BindParam(synth.param_lfo2_shape.ptr_);
            retrigger_.BindParam(synth.param_lfo2_retrigger.ptr_);
            phase_.BindParam(synth.param_lfo2_phase.ptr_);
            break;
        case 3:
            title_.setText("LFO3", juce::dontSendNotification);
            freq_.BindParam(synth.param_lfo3_freq.state_);
            shape_.BindParam(synth.param_lfo3_shape.ptr_);
            retrigger_.BindParam(synth.param_lfo3_retrigger.ptr_);
            phase_.BindParam(synth.param_lfo3_phase.ptr_);
            break;
        default:
            jassertfalse;
    }
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);
    addAndMakeVisible(freq_);
    addAndMakeVisible(shape_);
    addAndMakeVisible(retrigger_);
    addAndMakeVisible(phase_);
}

void LfoGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void LfoGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);
    auto b = content_bound;

    title_.setBounds(b.removeFromTop(20));

    auto w = b.getWidth() / 3;
    freq_.setBounds(b.removeFromLeft(w));
    shape_.setBounds(b.removeFromLeft(w));
    auto phase_bound = b;
    retrigger_.setBounds(phase_bound.removeFromTop(20));
    phase_.setBounds(phase_bound.removeFromTop(35));
}
}