#pragma once
#include "pluginshared/preset_panel.hpp"
#include "ui/osc1.hpp"
#include "ui/osc2.hpp"
#include "ui/osc3.hpp"
#include "ui/adsr.hpp"
#include "ui/filter.hpp"
#include "ui/lfo.hpp"
#include "ui/modulations.hpp"
#include "ui/delay.hpp"
#include "ui/chorus.hpp"
#include "ui/distortion.hpp"
#include "ui/reverb.hpp"

// ---------------------------------------- editor ----------------------------------------

class AnalogSynthAudioProcessor;

//==============================================================================
class AnalogSynthAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit AnalogSynthAudioProcessorEditor (AnalogSynthAudioProcessor&);
    ~AnalogSynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AnalogSynthAudioProcessor& p_;

    pluginshared::PresetPanel preset_;
    analogsynth::Osc1Gui osc1_;
    analogsynth::Osc2Gui osc2_;
    analogsynth::Osc3Gui osc3_;
    analogsynth::AdsrGui vol_env_;
    analogsynth::AdsrGui filter_env_;
    analogsynth::FilterGui filter_;
    analogsynth::LfoGui lfo1_;
    analogsynth::LfoGui lfo2_;
    analogsynth::LfoGui lfo3_;
    analogsynth::ModulationMatrixLayout modulations_;
    analogsynth::DelayGui delay_;
    analogsynth::ChorusGui chorus_;
    analogsynth::DistortionGui distortion_;
    analogsynth::ReverbGui reverb_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogSynthAudioProcessorEditor)
};
