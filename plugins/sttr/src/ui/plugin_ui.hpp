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

    ui::Dial stretchDial_{"Formant"};
    ui::Dial grainDial_{"Hop"};
    ui::Dial dryDelayDial_{"Dry Delay"};
    ui::Dial mixDial_{"Mix"};

    ui::Dial mulDial_{"nGrains"};
    ui::Dial betaDial_{"Harmonic"};
};
