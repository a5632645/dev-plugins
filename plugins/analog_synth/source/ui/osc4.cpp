#include "osc4.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
Osc4Gui::Osc4Gui(Synth& synth) {
    ui::SetLableBlack(title_);
    title_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title_);
    detune_.BindParam(synth.param_osc4_w0_detune.ptr_);
    addAndMakeVisible(detune_);
    volume_.BindParam(synth.param_osc4_vol.ptr_);
    addAndMakeVisible(volume_);
    ratio_.BindParam(synth.param_osc4_w_ratio.ptr_);
    addAndMakeVisible(ratio_);
    slope_.BindParam(synth.param_osc4_slope.ptr_);
    addAndMakeVisible(slope_);
    width_.BindParam(synth.param_osc4_width.ptr_);
    addAndMakeVisible(width_);
    label_shape_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(label_shape_);
    addAndMakeVisible(label_shape_);
    shape_.BindParam(synth.param_osc4_shape.ptr_);
    addAndMakeVisible(shape_);
    n_.BindParam(synth.param_osc4_n.ptr_);
    n_.SetTitleLayout(ui::FlatSlider::TitleLayout::Left);
    addAndMakeVisible(n_);
    use_max_n_.BindParam(synth.param_osc4_use_max_n.ptr_);
    addAndMakeVisible(use_max_n_);
}

void Osc4Gui::paint(juce::Graphics& g) {
    g.fillAll(ui::light_green_bg);
    auto content_bound = getLocalBounds().reduced(1);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void Osc4Gui::resized() {
    auto content_bound = getLocalBounds().reduced(1);

    title_.setBounds(content_bound.removeFromTop(20));
    
    auto shape_bound = content_bound.removeFromTop(25);
    label_shape_.setBounds(shape_bound.removeFromLeft(50));
    shape_.setBounds(shape_bound.reduced(2));

    auto line1 = content_bound.removeFromTop(65);
    auto w = line1.getWidth() / 3;
    volume_.setBounds(line1.removeFromLeft(w));
    detune_.setBounds(line1.removeFromLeft(w));
    ratio_.setBounds(line1.removeFromLeft(w));

    line1 = content_bound.removeFromTop(65);
    w = line1.getWidth() / 3;
    slope_.setBounds(line1.removeFromLeft(w));
    width_.setBounds(line1.removeFromLeft(w));

    line1 = content_bound;
    use_max_n_.setBounds(line1.removeFromRight(70).reduced(2));
    n_.setBounds(line1.reduced(2,1));
}
}