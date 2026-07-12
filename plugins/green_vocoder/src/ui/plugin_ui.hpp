#pragma once
#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

#include "widget/ensemble.hpp"
#include "widget/tracking.hpp"
#include "widget/pre_fx.hpp"
#include "widget/vocoder.hpp"

class AudioPluginAudioProcessor;

class PluginUi : public juce::Component {
public:
    static constexpr int kWidth = 750;
    static constexpr int kHeight = 350;

    explicit PluginUi(AudioPluginAudioProcessor& p);

    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    pluginshared::PresetPanel preset_panel_;

    green_vocoder::widget::PreFx pre_fx_;
    green_vocoder::widget::Vocoder vocoder_;
    green_vocoder::widget::Ensemble ensemble_;
    green_vocoder::widget::Tracking tracking_;
};
