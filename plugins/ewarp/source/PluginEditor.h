#pragma once
#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

// ---------------------------------------- editor ----------------------------------------

class EwarpAudioProcessor;

//==============================================================================
class EwarpAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit EwarpAudioProcessorEditor (EwarpAudioProcessor&);
    ~EwarpAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EwarpAudioProcessor& p_;
    pluginshared::PresetPanel preset_;

    ui::Dial warp_{"warp"};
    ui::Dial ratio_{"ratio"};
    ui::Dial am2rm_{"am2rm"};
    ui::Dial decay_{"decay"};
    ui::Dial reverse_{"reverse"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EwarpAudioProcessorEditor)
};
