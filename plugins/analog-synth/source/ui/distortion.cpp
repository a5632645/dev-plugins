#include "distortion.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
DistortionGui::DistortionGui(Synth& synth) {
    enable_.BindParam(synth.param_distortion_enable.ptr_);
    addAndMakeVisible(enable_);
    ui::SetLableBlack(title_);
    title_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title_);

    drive_.BindParam(synth.param_distortion_drive.ptr_);
    addAndMakeVisible(drive_);
}

void DistortionGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void DistortionGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);

    auto top_bound = content_bound.removeFromTop(25);
    enable_.setBounds(top_bound.removeFromLeft(25).reduced(2));
    title_.setBounds(top_bound.removeFromLeft(100));

    auto line1 = content_bound.removeFromTop(65);
    auto w = line1.getWidth() / 3;
    drive_.setBounds(line1.removeFromLeft(w));
}
}