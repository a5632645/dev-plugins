#include "noise.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
NoiseGui::NoiseGui(Synth& synth) {
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);

    volume_.BindParam(synth.param_noise_vol.ptr_);
    addAndMakeVisible(volume_);
    type_.BindParam(synth.param_noise_type.ptr_, true);
    addAndMakeVisible(type_);
}

void NoiseGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void NoiseGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);
    
    title_.setBounds(content_bound.removeFromTop(20));

    type_.setBounds(content_bound.removeFromTop(25));
    {
        auto& cubes = type_.GetAllCubes();
        auto bound = type_.getLocalBounds();
        auto w = bound.getWidth() / static_cast<int>(cubes.size());
        for (auto& cube : cubes) {
            cube->setBounds(bound.removeFromLeft(w).reduced(2));
        }
    }

    auto line = content_bound;
    auto w = line.getWidth() / 3;
    volume_.setBounds(line.removeFromLeft(w));
}
}