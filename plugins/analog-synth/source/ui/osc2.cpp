#include "osc2.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
Osc2Gui::Osc2Gui(Synth& synth) {
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);
    detune_.BindParam(synth.param_osc2_detune.ptr_);
    addAndMakeVisible(detune_);
    volume_.BindParam(synth.param_osc2_vol.ptr_);
    addAndMakeVisible(volume_);
    pwm_.BindParam(synth.param_osc2_pwm.ptr_);
    addAndMakeVisible(pwm_);
    waveform_.BindParam(synth.param_osc2_shape.ptr_, true);
    addAndMakeVisible(waveform_);
    sync_.BindParam(synth.param_osc2_sync.ptr_);
    addAndMakeVisible(sync_);
}

void Osc2Gui::paint(juce::Graphics& g) {
    g.fillAll(ui::light_green_bg);
    auto content_bound = getLocalBounds().reduced(1);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void Osc2Gui::resized() {
    auto content_bound = getLocalBounds().reduced(1);
    auto b = content_bound;

    title_.setBounds(b.removeFromTop(20));
    
    waveform_.setBounds(b.removeFromTop(25));
    {
        auto& cubes = waveform_.GetAllCubes();
        auto bound = waveform_.getLocalBounds();
        auto w = bound.getWidth() / static_cast<int>(cubes.size());
        for (auto& cube : cubes) {
            cube->setBounds(bound.removeFromLeft(w).reduced(2));
        }
    }

    auto w = (b.getWidth() - 20) / 3;
    detune_.setBounds(b.removeFromLeft(w));
    volume_.setBounds(b.removeFromLeft(w));
    pwm_.setBounds(b.removeFromLeft(w));
    sync_.setBounds(b.removeFromLeft(20).withSizeKeepingCentre(20, 20));
}
}