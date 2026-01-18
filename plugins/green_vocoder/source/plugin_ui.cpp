#include "plugin_ui.hpp"

#include "PluginProcessor.h"

PluginUi::PluginUi(AudioPluginAudioProcessor& p)
    : preset_panel_(*p.preset_manager_)
    , pre_fx_(p)
    , vocoder_(p)
    , ensemble_(p)
    , tracking_(p)
{
    addAndMakeVisible(preset_panel_);
    addAndMakeVisible(pre_fx_);
    addAndMakeVisible(vocoder_);
    addAndMakeVisible(ensemble_);
    addAndMakeVisible(tracking_);
}

void PluginUi::paint (juce::Graphics& g) {
    g.fillAll(ui::black_bg);
    g.setColour(ui::green_bg);
    g.fillRect(preset_panel_.getBounds());
    g.fillRect(pre_fx_.getBounds());
    g.fillRect(vocoder_.getBounds());
    g.fillRect(tracking_.getBounds());
    g.fillRect(ensemble_.getBounds());
}

void PluginUi::resized()
{
    auto b = getLocalBounds();
    preset_panel_.setBounds(b.removeFromTop(30));
    auto left_panel = b.removeFromLeft(260);
    pre_fx_.setBounds(left_panel.removeFromTop(90).reduced(1));
    tracking_.setBounds(left_panel.removeFromTop(90).reduced(1));
    ensemble_.setBounds(left_panel.removeFromTop(90).reduced(1));
    auto right_panel = b;
    vocoder_.setBounds(right_panel.reduced(1));
}
