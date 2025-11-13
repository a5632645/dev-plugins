#include "marcos.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
MarcosGui::MarcosGui(Synth& synth) {
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);

    marco1_.BindParam(synth.param_marco1.ptr_);
    addAndMakeVisible(marco1_);
    marco2_.BindParam(synth.param_marco2.ptr_);
    addAndMakeVisible(marco2_);
    marco3_.BindParam(synth.param_marco3.ptr_);
    addAndMakeVisible(marco3_);
    marco4_.BindParam(synth.param_marco4.ptr_);
    addAndMakeVisible(marco4_);
}

void MarcosGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void MarcosGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);
    
    title_.setBounds(content_bound.removeFromTop(20));

    auto b = content_bound.removeFromTop(65);
    auto w = b.getWidth() / 4;
    marco1_.setBounds(b.removeFromLeft(w));
    marco2_.setBounds(b.removeFromLeft(w));
    marco3_.setBounds(b.removeFromLeft(w));
    marco4_.setBounds(b.removeFromLeft(w));
}
}