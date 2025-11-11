#include "delay.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
DelayGui::DelayGui(Synth& synth) {
    enable_.BindParam(synth.param_delay_enable.ptr_);
    addAndMakeVisible(enable_);
    ui::SetLableBlack(title_);
    title_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title_);
    pingpong_.BindParam(synth.param_delay_pingpong.ptr_);
    addAndMakeVisible(pingpong_);
    delay_.BindParam(synth.param_delay_ms.ptr_);
    addAndMakeVisible(delay_);
    lowcut_.BindParam(synth.param_delay_lp.ptr_);
    addAndMakeVisible(lowcut_);
    highcut_.BindParam(synth.param_delay_hp.ptr_);
    addAndMakeVisible(highcut_);
    feedback_.BindParam(synth.param_delay_feedback.ptr_);
    addAndMakeVisible(feedback_);
    mix_.BindParam(synth.param_delay_mix.ptr_);
    addAndMakeVisible(mix_);
}

void DelayGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void DelayGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);

    auto top_bound = content_bound.removeFromTop(25);
    enable_.setBounds(top_bound.removeFromLeft(25).reduced(2));
    pingpong_.setBounds(top_bound.removeFromRight(100).reduced(2));
    title_.setBounds(top_bound);

    auto line1 = content_bound.removeFromTop(70);
    auto w = line1.getWidth() / 3;
    delay_.setBounds(line1.removeFromLeft(w));
    feedback_.setBounds(line1.removeFromLeft(w));
    mix_.setBounds(line1.removeFromLeft(w));

    auto line2 = content_bound.removeFromTop(70);
    w = line2.getWidth() / 3;
    lowcut_.setBounds(line2.removeFromLeft(w));
    highcut_.setBounds(line2.removeFromLeft(w));
}
}