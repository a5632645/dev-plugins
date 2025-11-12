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

    auto w = content_bound.getWidth() / 3;
    detune_.setBounds(content_bound.removeFromLeft(w));
    volume_.setBounds(content_bound.removeFromLeft(w));
    pwm_.setBounds(content_bound.removeFromLeft(w));
}
}