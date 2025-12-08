#include "phaser.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
PhaserGui::PhaserGui(Synth& synth) {
    enable_.BindParam(synth.param_phaser_enable.ptr_);
    addAndMakeVisible(enable_);
    ui::SetLableBlack(title_);
    title_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title_);

    mix_.BindParam(synth.param_phaser_mix.ptr_);
    addAndMakeVisible(mix_);
    center_.BindParam(synth.param_phaser_center.ptr_);
    addAndMakeVisible(center_);
    depth_.BindParam(synth.param_phaser_depth.ptr_);
    addAndMakeVisible(depth_);
    rate_.BindParam(synth.param_phaser_rate.state_);
    addAndMakeVisible(rate_);
    feedback_.BindParam(synth.param_phase_feedback.ptr_);
    addAndMakeVisible(feedback_);
    Q_.BindParam(synth.param_phaser_Q.ptr_);
    addAndMakeVisible(Q_);
    stereo_.BindParam(synth.param_phaser_stereo.ptr_);
    addAndMakeVisible(stereo_);
}

void PhaserGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void PhaserGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);

    auto top_bound = content_bound.removeFromTop(25);
    enable_.setBounds(top_bound.removeFromLeft(25).reduced(2));
    title_.setBounds(top_bound.removeFromLeft(100));

    auto line1 = content_bound.removeFromTop(65);
    auto w = line1.getWidth() / 4;
    center_.setBounds(line1.removeFromLeft(w));
    depth_.setBounds(line1.removeFromLeft(w));
    rate_.setBounds(line1.removeFromLeft(w));
    mix_.setBounds(line1.removeFromLeft(w));

    auto line2 = content_bound.removeFromTop(65);
    w = line2.getWidth() / 4;
    feedback_.setBounds(line2.removeFromLeft(w));
    Q_.setBounds(line2.removeFromLeft(w));
    stereo_.setBounds(line2.removeFromLeft(w));
}
}