#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

//==============================================================================
// PluginUi
//==============================================================================
PluginUi::PluginUi(VitalChorusAudioProcessor& p)
    : preset_(*p.preset_manager_)
    , p_(p)
    , chorus_view_(p)
    , filter_view_(p)
{
    addAndMakeVisible(preset_);

    auto& apvts = *p.value_tree_;

    lfo_dial_.BindParam(p_.params_.freq);
    addAndMakeVisible(lfo_dial_);

    depth_.BindParam(apvts, "depth");
    addAndMakeVisible(depth_);
    delay1_.BindParam(apvts, "delay1");
    addAndMakeVisible(delay1_);
    delay2_.BindParam(apvts, "delay2");
    addAndMakeVisible(delay2_);
    feedback_.BindParam(apvts, "feedback");
    addAndMakeVisible(feedback_);
    mix_.BindParam(apvts, "mix");
    addAndMakeVisible(mix_);
    cutoff_.BindParam(apvts, "cutoff");
    cutoff_.slider.onValueChange = [this] {
        filter_view_.repaint();
    };
    addAndMakeVisible(cutoff_);
    spread_.slider.onValueChange = [this] {
        filter_view_.repaint();
    };
    spread_.BindParam(apvts, "spread");
    addAndMakeVisible(spread_);
    num_voices_.BindParam(apvts, "num_voices");
    addAndMakeVisible(num_voices_);

    addAndMakeVisible(chorus_view_);
    addAndMakeVisible(filter_view_);

    setSize(kWidth, kHeight);
}

PluginUi::~PluginUi() = default;

//==============================================================================
void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(std::max(30, b.proportionOfHeight(0.1f))));

    auto w = b.getWidth() / 3;
    {
        auto left = b.removeFromLeft(w);
        auto top = left.removeFromTop(left.getHeight() / 2);
        num_voices_.setBounds(top.removeFromLeft(top.getWidth() / 2));
        lfo_dial_.setBounds(top);
        auto bottom = left;
        auto w2 = bottom.getWidth() / 3;
        depth_.setBounds(bottom.removeFromLeft(w2).reduced(1, 0));
        delay1_.setBounds(bottom.removeFromLeft(w2).reduced(1, 0));
        delay2_.setBounds(bottom.reduced(1, 0));
    }
    {
        auto center = b.removeFromLeft(w);
        chorus_view_.setBounds(center.removeFromTop(center.getHeight() / 2).reduced(2, 2));
        filter_view_.setBounds(center.reduced(2, 2));
    }
    {
        auto right = b;
        auto top = right.removeFromTop(right.getHeight() / 2);
        feedback_.setBounds(top.removeFromLeft(top.getWidth() / 2));
        mix_.setBounds(top);
        auto bottom = right;
        cutoff_.setBounds(bottom.removeFromLeft(bottom.getWidth() / 2));
        spread_.setBounds(bottom);
    }
}
