#pragma once
#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

#include "../global.hpp"

#include "pre_fx.hpp"
#include "tracking.hpp"
#include "vocoder.hpp"

class AudioPluginAudioProcessor;

namespace green_vocoder::ui {

class PluginUi : public juce::Component {
public:
    explicit PluginUi(AudioPluginAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    pluginshared::PresetPanel preset_panel_;

    PreFx pre_fx_;
    Vocoder vocoder_;
    Tracking tracking_;
};

} // namespace green_vocoder::ui
