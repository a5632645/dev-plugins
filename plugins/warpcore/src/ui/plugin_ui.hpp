#pragma once
#include "pluginshared/component.hpp"
#include "pluginshared/preset_panel.hpp"

class EmptyAudioProcessor;

class PluginUi : public juce::Component {
public:
    static constexpr int kWidth = 480;
    static constexpr int kHeight = 320;

    explicit PluginUi(EmptyAudioProcessor& p);
    ~PluginUi() override;

    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    void TrySetSize(int width, int height);
    
    juce::TooltipWindow tooltip_;
    pluginshared::PresetPanel preset_;

    ui::Dial warp_{"Warp"};
    ui::Dial f_low_{"Scale"};
    ui::Dial f_high_{"Freq High"};
    ui::Dial poles_{"Poles"};
    ui::Dial pitch_{"Pitch"};
    ui::Dial drywet_{"DryWet"};

    juce::Label pitch_affect_label{"", "Formant Mode"};
    ui::Switch pitch_affect_{"Formant", "Pitch"};
    ui::Switch fill_gap_{"FillGap"};

    juce::Label freq_label_{"", "Freq Mode"};
    ui::FlatCombobox freq_mode_{};
};
