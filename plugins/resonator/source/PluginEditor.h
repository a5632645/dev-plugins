#pragma once
#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

#include "ui/bar.hpp"
#include "ui/couple.hpp"
#include "ui/resonator.hpp"

class ResonatorAudioProcessor;

class ResonatorAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit ResonatorAudioProcessorEditor (ResonatorAudioProcessor&);
    ~ResonatorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ResonatorAudioProcessor& p_;
    pluginshared::PresetPanel preset_panel_;

    BarGui bar_;
    CoupleGui couple_;
    ResonatorGui resonators_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorAudioProcessorEditor)
};
