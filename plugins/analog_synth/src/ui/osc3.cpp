#include "osc3.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
Osc3Gui::Osc3Gui(Synth& synth) {
    ui::SetLableBlack(title_);
    title_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title_);
    detune_.BindParam(synth.param_osc3_detune.ptr_);
    addAndMakeVisible(detune_);
    volume_.BindParam(synth.param_osc3_vol.ptr_);
    addAndMakeVisible(volume_);
    pwm_.BindParam(synth.param_osc3_pwm.ptr_);
    addAndMakeVisible(pwm_);
    waveform_.BindParam(synth.param_osc3_shape.ptr_, true);
    addAndMakeVisible(waveform_);
    unison_.BindParam(synth.param_osc3_unison.ptr_);
    addAndMakeVisible(unison_);
    unison_detune_.BindParam(synth.param_osc3_unison_detune.ptr_);
    addAndMakeVisible(unison_detune_);
    unison_type_.BindParam(synth.param_osc3_uniso_type.ptr_);
    addAndMakeVisible(unison_type_);
    retrigger_.BindParam(synth.param_osc3_retrigger.ptr_);
    addAndMakeVisible(retrigger_);
    phase_.BindParam(synth.param_osc3_phase.ptr_);
    addAndMakeVisible(phase_);
    random_.BindParam(synth.param_osc3_phase_random.ptr_);
    addAndMakeVisible(random_);
}

void Osc3Gui::paint(juce::Graphics& g) {
    g.fillAll(ui::light_green_bg);
    auto content_bound = getLocalBounds().reduced(1);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void Osc3Gui::resized() {
    auto content_bound = getLocalBounds().reduced(1);

    title_.setBounds(content_bound.removeFromTop(20));
    
    waveform_.setBounds(content_bound.removeFromTop(25));
    {
        auto& cubes = waveform_.GetAllCubes();
        auto bound = waveform_.getLocalBounds();
        auto w = bound.getWidth() / static_cast<int>(cubes.size());
        for (auto& cube : cubes) {
            cube->setBounds(bound.removeFromLeft(w).reduced(2));
        }
    }

    auto unison_bound = content_bound.removeFromTop(40);
    unison_.setBounds(unison_bound.removeFromLeft(unison_bound.getWidth() / 2).reduced(2));
    unison_detune_.setBounds(unison_bound.reduced(2));

    auto type_bound = content_bound.removeFromTop(25);
    unison_type_.setBounds(type_bound.removeFromLeft(type_bound.getWidth() / 2).reduced(2));
    retrigger_.setBounds(type_bound.reduced(2));

    auto phase_bound = content_bound.removeFromTop(40);
    phase_.setBounds(phase_bound.removeFromLeft(phase_bound.getWidth() / 2).reduced(2));
    random_.setBounds(phase_bound.reduced(2));

    auto dial_bound = content_bound;
    auto w = dial_bound.getWidth() / 3;
    detune_.setBounds(dial_bound.removeFromLeft(w));
    volume_.setBounds(dial_bound.removeFromLeft(w));
    pwm_.setBounds(dial_bound.removeFromLeft(w));
}
}