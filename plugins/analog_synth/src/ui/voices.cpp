#include "voices.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
VoicesGui::VoicesGui(Synth& synth) {
    title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(title_);
    addAndMakeVisible(title_);

    legato_.BindParam(synth.param_legato.ptr_);
    addAndMakeVisible(legato_);
    time_.BindParam(synth.param_glide_time.ptr_);
    addAndMakeVisible(time_);
    voice_.BindParam(synth.param_num_voices.ptr_);
    addAndMakeVisible(voice_);
    voice_title_.setJustificationType(juce::Justification::centredLeft);
    ui::SetLableBlack(voice_title_);
    addAndMakeVisible(voice_title_);
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

    time_.setBounds(content_bound.removeFromLeft(65));
    auto line = content_bound.removeFromLeft(80);
    legato_.setBounds(line.removeFromTop(28).reduced(2));
    voice_title_.setBounds(line.removeFromTop(static_cast<int>(voice_title_.getFont().getHeight())));
    voice_.setBounds(line);
}
}
