#include "pre_fx.hpp"
#include "PluginProcessor.h"
#include "param_ids.hpp"

namespace green_vocoder::widget {
PreFx::PreFx(AudioPluginAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(title_);
    tilt_.BindParam(apvts, id::kPreTilt);
    addAndMakeVisible(tilt_);
    swap_.BindParam(apvts, id::kChannelSwap);
    addAndMakeVisible(swap_);
    pitch_ch_.BindParam(apvts, id::kPitchChannel);
    addAndMakeVisible(pitch_ch_);

    swap_title_.setJustificationType(juce::Justification::left);
    ui::SetLableBlack(swap_title_);
    addAndMakeVisible(swap_title_);
    
    pitch_title_.setJustificationType(juce::Justification::left);
    ui::SetLableBlack(pitch_title_);
    addAndMakeVisible(pitch_title_);
}

void PreFx::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));

    b.removeFromLeft(70);
    auto channel_bound = b.removeFromLeft(100);
    swap_.setBounds(channel_bound.removeFromTop(channel_bound.getHeight() / 2).reduced(2));
    pitch_ch_.setBounds(channel_bound.reduced(2));

    swap_title_.setBounds(swap_.getBounds().translated(-70, 0));
    pitch_title_.setBounds(pitch_ch_.getBounds().translated(-70, 0));

    tilt_.setBounds(b.removeFromLeft(50).withHeight(65));
}
}
