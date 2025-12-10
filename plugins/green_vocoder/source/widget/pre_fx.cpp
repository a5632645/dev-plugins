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
    saturation_.BindParam(p.output_saturation_);
    addAndMakeVisible(saturation_);
    drive_.BindParam(p.output_drive_);
    addAndMakeVisible(drive_);
}

void PreFx::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));

    auto channel_bound = b.removeFromLeft(100);
    main_route_.setBounds(channel_bound.removeFromTop(channel_bound.getHeight() / 2));
    side_route_.setBounds(channel_bound);

    tilt_.setBounds(b.removeFromLeft(50).withHeight(65));

    saturation_.setBounds(b.removeFromLeft(50).withHeight(30));
    drive_.setBounds(b.removeFromLeft(50));
}
}
