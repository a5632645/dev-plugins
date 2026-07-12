#include "pre_fx.hpp"
#include "PluginProcessor.h"
#include "param_ids.hpp"

namespace green_vocoder::widget {
PreFx::PreFx(AudioPluginAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(title_);
    tilt_.BindParam(apvts, id::kPreTilt);
    addAndMakeVisible(tilt_);
    main_route_.BindParam(apvts, id::kMainChannelConfig);
    addAndMakeVisible(main_route_);
    side_route_.BindParam(apvts, id::kSideChannelConfig);
    addAndMakeVisible(side_route_);

    main_route_title_.setJustificationType(juce::Justification::left);
    ui::SetLableBlack(main_route_title_);
    addAndMakeVisible(main_route_title_);
    
    side_route_title_.setJustificationType(juce::Justification::left);
    ui::SetLableBlack(side_route_title_);
    addAndMakeVisible(side_route_title_);
}

void PreFx::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));

    b.removeFromLeft(70);
    auto channel_bound = b.removeFromLeft(100);
    main_route_.setBounds(channel_bound.removeFromTop(channel_bound.getHeight() / 2).reduced(2));
    side_route_.setBounds(channel_bound.reduced(2));

    main_route_title_.setBounds(main_route_.getBounds().translated(-70, 0));
    side_route_title_.setBounds(side_route_.getBounds().translated(-70, 0));

    tilt_.setBounds(b.removeFromLeft(50).withHeight(65));
}
}
