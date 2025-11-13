#include "voices.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
VoicesGui::VoicesGui(Synth& synth) {
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);

    addAndMakeVisible(legato_);
    addAndMakeVisible(porta_);
    addAndMakeVisible(time_);
    addAndMakeVisible(mono_);
    voice_.SetTitleLayout(ui::FlatSlider::TitleLayout::Top);
    addAndMakeVisible(voice_);
}

void VoicesGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void VoicesGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);
    
    title_.setBounds(content_bound.removeFromTop(20));

    time_.setBounds(content_bound.removeFromRight(50));
    auto line1 = content_bound.removeFromTop(25);
    auto w = line1.getWidth() / 2;
    legato_.setBounds(line1.removeFromLeft(w).reduced(2));
    porta_.setBounds(line1.removeFromLeft(w).reduced(2));

    auto line2 = content_bound;
    mono_.setBounds(line2.removeFromLeft(w).removeFromBottom(28).reduced(3));
    voice_.setBounds(line2.removeFromLeft(w).reduced(2));
}
}