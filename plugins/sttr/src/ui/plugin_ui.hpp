#pragma once

#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

class SttrAudioProcessor;

class PluginUi : public juce::Component {
public:
    static constexpr int kWidth = 300;
    static constexpr int kHeight = 150;

    explicit PluginUi(SttrAudioProcessor& p);

    void resized() override;
private:
    pluginshared::PresetPanel preset_;

    ui::Dial stretchDial_{"Stretch"};
    ui::Dial grainDial_{"Hop"};
    ui::Dial dryDelayDial_{"Dry Delay"};
    ui::Dial mixDial_{"Mix"};

    ui::FlatCombobox windowTypeCombo_;
    juce::Label windowLabel_;
};
