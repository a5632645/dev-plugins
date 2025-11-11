#include "reverb.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
ReverbGui::ReverbGui(Synth& synth) {
    enable_.BindParam(synth.param_reverb_enable.ptr_);
    addAndMakeVisible(enable_);
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);

    mix_.BindParam(synth.param_reverb_mix.ptr_);
    addAndMakeVisible(mix_);
    predelay_.BindParam(synth.param_reverb_predelay.ptr_);
    addAndMakeVisible(predelay_);
    lowpass_.BindParam(synth.param_reverb_lowpass.ptr_);
    addAndMakeVisible(lowpass_);
    decay_.BindParam(synth.param_reverb_decay.ptr_);
    addAndMakeVisible(decay_);
    size_.BindParam(synth.param_reverb_size.ptr_);
    addAndMakeVisible(size_);
    damp_.BindParam(synth.param_reverb_damp.ptr_);
    addAndMakeVisible(damp_);
}

void ReverbGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void ReverbGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);

    auto top_bound = content_bound.removeFromTop(25);
    enable_.setBounds(top_bound.removeFromLeft(25).reduced(2));
    title_.setBounds(top_bound);

    auto line1 = content_bound.removeFromTop(70);
    auto w = line1.getWidth() / 3;
    size_.setBounds(line1.removeFromLeft(w));
    decay_.setBounds(line1.removeFromLeft(w));
    mix_.setBounds(line1.removeFromLeft(w));

    auto line2 = content_bound.removeFromTop(70);
    w = line2.getWidth() / 3;
    lowpass_.setBounds(line2.removeFromLeft(w));
    predelay_.setBounds(line2.removeFromLeft(w));
    damp_.setBounds(line2.removeFromLeft(w));
}
}