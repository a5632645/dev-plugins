#include "filter.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
FilterGui::FilterGui(Synth& synth) {
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);
    type_.BindParam(synth.filter_.param_type.ptr_);
    type_.onChange = [this] {OnFilterTypeChanged();};
    addAndMakeVisible(type_);
    cutoff_.BindParam(synth.filter_.param_cutoff.ptr_);
    addAndMakeVisible(cutoff_);
    morph_.BindParam(synth.filter_.param_morph.ptr_);
    addAndMakeVisible(morph_);
    mix_.BindParam(synth.filter_.param_mix.ptr_);
    addAndMakeVisible(mix_);
    var1_.BindParam(synth.filter_.param_var1.ptr_);
    addAndMakeVisible(var1_);
    var2_.BindParam(synth.filter_.param_var2.ptr_);
    addAndMakeVisible(var2_);
    var3_.BindParam(synth.filter_.param_var3.ptr_);
    addAndMakeVisible(var3_);
    enable_.BindParam(synth.filter_.param_enable.ptr_);
    addAndMakeVisible(enable_);
    resonance_.BindParam(synth.filter_.param_resonance.ptr_);
    addAndMakeVisible(resonance_);

    OnFilterTypeChanged();
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
    title_.setBounds(top_bound.removeFromLeft(50));
    type_.setBounds(top_bound.reduced(2));

    auto top = content_bound.removeFromTop(content_bound.getHeight() / 2);
    auto top_w = top.getWidth() / 4;
    cutoff_.setBounds(top.removeFromLeft(top_w));
    resonance_.setBounds(top.removeFromLeft(top_w));
    morph_.setBounds(top.removeFromLeft(top_w));

    auto bottom = content_bound;
    auto bottom_w = bottom.getWidth() / 4;
    mix_.setBounds(bottom.removeFromLeft(bottom_w));
    var1_.setBounds(bottom.removeFromLeft(bottom_w));
    var2_.setBounds(bottom.removeFromLeft(bottom_w));
    var3_.setBounds(bottom.removeFromLeft(bottom_w));
}

void FilterGui::OnFilterTypeChanged() {
    switch (type_.getSelectedItemIndex()) {
        case analogsynth::Filter::FilterType_SVF:
            var1_.setVisible(false);
            var2_.setVisible(false);
            var3_.setVisible(false);
            break;
        case analogsynth::Filter::FilterType_Ladder4:
            var1_.setVisible(true);
            var1_.label.setText("Q", juce::dontSendNotification);
            var2_.setVisible(false);
            var3_.setVisible(false);
            break;
        case analogsynth::Filter::FilterType_Ladder8:
            var1_.setVisible(true);
            var1_.label.setText("Q", juce::dontSendNotification);
            var2_.setVisible(false);
            var3_.setVisible(false);
            break;
        case analogsynth::Filter::FilterType_SallenKey:
            var1_.setVisible(true);
            var1_.label.setText("Q", juce::dontSendNotification);
            var2_.setVisible(false);
            var3_.setVisible(false);
            break;
        default:
            jassertfalse;
    }
}
}