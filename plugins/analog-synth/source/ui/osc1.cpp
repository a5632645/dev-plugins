#include "osc1.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
Osc1Gui::Osc1Gui(Synth& synth) {
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);
    detune_.BindParam(synth.param_osc1_detune.ptr_);
    addAndMakeVisible(detune_);
    volume_.BindParam(synth.param_osc1_vol.ptr_);
    addAndMakeVisible(volume_);
    pwm_.BindParam(synth.param_osc1_pwm.ptr_);
    addAndMakeVisible(pwm_);
    waveform_.BindParam(synth.param_osc1_shape.ptr_, true);
    addAndMakeVisible(waveform_);
}

void Osc1Gui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void Osc1Gui::resized() {
    auto content_bound = getLocalBounds().reduced(1);
    auto const ref = content_bound;
    auto b = ref;

    title_.setBounds(b.removeFromTop(20));
    
    waveform_.setBounds(b.removeFromTop(25));
    {
        auto& cubes = waveform_.GetAllCubes();
        auto bound = waveform_.getLocalBounds();
        auto w = bound.getWidth() / cubes.size();
        for (auto& cube : cubes) {
            cube->setBounds(bound.removeFromLeft(w).reduced(2));
        }
    }

    detune_.setBounds(b.removeFromLeft(ref.proportionOfWidth(0.3f)));
    volume_.setBounds(b.removeFromLeft(ref.proportionOfWidth(0.3f)));
    pwm_.setBounds(b.removeFromLeft(ref.proportionOfWidth(0.3f)));
}
}