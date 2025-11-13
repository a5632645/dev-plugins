#include "filter.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
FilterGui::FilterGui(Synth& synth) {
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);
    cutoff_.BindParam(synth.param_cutoff_pitch.ptr_);
    addAndMakeVisible(cutoff_);
    direct_.BindParam(synth.param_filter_direct.ptr_);
    addAndMakeVisible(direct_);
    Q_.BindParam(synth.param_Q.ptr_);
    addAndMakeVisible(Q_);
    lp_.BindParam(synth.param_filter_lp.ptr_);
    addAndMakeVisible(lp_);
    hp_.BindParam(synth.param_filter_hp.ptr_);
    addAndMakeVisible(hp_);
    bp_.BindParam(synth.param_filter_bp.ptr_);
    addAndMakeVisible(bp_);
    enable_.BindParam(synth.param_filter_enable.ptr_);
    addAndMakeVisible(enable_);
}

void FilterGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void FilterGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);

    auto top_bound = content_bound.removeFromTop(25);
    enable_.setBounds(top_bound.removeFromLeft(25).reduced(2));
    title_.setBounds(top_bound.removeFromLeft(100));

    auto top = content_bound.removeFromTop(content_bound.getHeight() / 2);
    auto top_w = top.getWidth() / 4;
    cutoff_.setBounds(top.removeFromLeft(top_w));
    Q_.setBounds(top.removeFromLeft(top_w));

    auto bottom = content_bound;
    auto bottom_w = bottom.getWidth() / 4;
    direct_.setBounds(bottom.removeFromLeft(bottom_w));
    lp_.setBounds(bottom.removeFromLeft(bottom_w));
    bp_.setBounds(bottom.removeFromLeft(bottom_w));
    hp_.setBounds(bottom.removeFromLeft(bottom_w));
}
}