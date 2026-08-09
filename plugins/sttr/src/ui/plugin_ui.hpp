#pragma once

#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

class SttrAudioProcessor;

class PluginUi : public juce::Component {
public:
    static constexpr int kWidth = 300;
    static constexpr int kHeight = 200;

    explicit PluginUi(SttrAudioProcessor& p);

    void resized() override;
private:
    pluginshared::PresetPanel preset_;

    ui::Dial stretch_dial_{"Formant"};
    ui::Dial grain_dial_{"Hop"};
    ui::Dial dry_delay_dial_{"Dry Delay"};
    ui::Dial mix_dial_{"Mix"};

    ui::Dial mul_dial_{"nGrains"};
    ui::Dial beta_dial_{"Harmonic"};

    ui::Switch reverse_switch_{"Reverse", "Forward"};
};
