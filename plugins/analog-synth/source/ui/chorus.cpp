#include "chorus.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
ChorusGui::ChorusGui(Synth& synth) {
    enable_.BindParam(synth.param_chorus_enable.ptr_);
    addAndMakeVisible(enable_);
    ui::SetLableBlack(title_);
    title_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title_);

    delay_.BindParam(synth.param_chorus_delay.ptr_);
    addAndMakeVisible(delay_);
    depth_.BindParam(synth.param_chorus_depth.ptr_);
    addAndMakeVisible(depth_);
    rate_.BindParam(synth.param_chorus_rate.state_);
    addAndMakeVisible(rate_);
    feedback_.BindParam(synth.param_chorus_feedback.ptr_);
    addAndMakeVisible(feedback_);
    mix_.BindParam(synth.param_chorus_mix.ptr_);
    addAndMakeVisible(mix_);
}

void ChorusGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void ChorusGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);

    auto top_bound = content_bound.removeFromTop(25);
    enable_.setBounds(top_bound.removeFromLeft(25).reduced(2));
    title_.setBounds(top_bound);

    auto line1 = content_bound.removeFromTop(70);
    auto w = line1.getWidth() / 3;
    rate_.setBounds(line1.removeFromLeft(w));
    delay_.setBounds(line1.removeFromLeft(w));
    depth_.setBounds(line1.removeFromLeft(w));

    auto line2 = content_bound.removeFromTop(70);
    w = line2.getWidth() / 3;
    feedback_.setBounds(line2.removeFromLeft(w));
    mix_.setBounds(line2.removeFromLeft(w));
}
}