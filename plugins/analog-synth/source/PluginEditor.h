#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <pluginshared/preset_panel.hpp>
#include "ui/osc1.hpp"
#include "ui/osc2.hpp"
#include "ui/osc3.hpp"
#include "ui/osc4.hpp"
#include "ui/noise.hpp"
#include "ui/adsr.hpp"
#include "ui/filter.hpp"
#include "ui/lfo.hpp"
#include "ui/modulations.hpp"
#include "ui/fx_chain.hpp"
#include "ui/marcos.hpp"
#include "ui/voices.hpp"

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
    analogsynth::Osc4Gui osc4_;
    analogsynth::NoiseGui noise_;
    analogsynth::AdsrGui vol_env_;
    analogsynth::AdsrGui filter_env_;
    analogsynth::FilterGui filter_;
    analogsynth::LfoGui lfo1_;
    analogsynth::LfoGui lfo2_;
    analogsynth::LfoGui lfo3_;
    analogsynth::ModulationMatrixLayout modulations_;
    analogsynth::FxChainGui fx_chain_;
    analogsynth::MarcosGui marcos_;
    analogsynth::VoicesGui voices_;
    
    juce::MidiKeyboardComponent midi_component_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogSynthAudioProcessorEditor)
};
