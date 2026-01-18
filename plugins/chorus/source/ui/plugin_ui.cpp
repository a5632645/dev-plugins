#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

PluginUi::PluginUi(EmptyAudioProcessor& p)
    : preset_(*p.preset_manager_)
{
    detune_.BindParam(p.detune_.ptr_);
    addAndMakeVisible(detune_);
    spread_.BindParam(p.pan_.ptr_);
    addAndMakeVisible(spread_);
}

void PluginUi::resized()
{
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(30));

    auto line = b.removeFromTop(65);
    detune_.setBounds(line.removeFromLeft(50));
    spread_.setBounds(line.removeFromLeft(50));
}
