#pragma once
#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

class EmptyAudioProcessor;

class PluginUi : public juce::Component {
public:
    static constexpr int kWidth = 500;
    static constexpr int kHeight = 400;

    explicit PluginUi(EmptyAudioProcessor& p);

    void resized() override;
private:
    pluginshared::PresetPanel preset_;

    ui::Dial detune_{"detune"};
    ui::Dial spread_{"spread"};
};
