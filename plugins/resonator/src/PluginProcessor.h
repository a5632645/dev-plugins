#pragma once
#include <pluginshared/juce_param_listener.hpp>
#include <pluginshared/preset_manager.hpp>

#include "poly_manager.hpp"
#include "resonator.hpp"

// ---------------------------------------- juce processor ----------------------------------------
class ResonatorAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    ResonatorAudioProcessor();
    ~ResonatorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    JuceParamListener param_listener_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;
    std::unique_ptr<pluginshared::PresetManager> preset_manager_;

    std::array<juce::AudioParameterFloat*, kNumResonators> pitches_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> fine_tune_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> damp_pitch_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> damp_gain_db_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> dispersion_pole_radius_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> decays_{};
    std::array<juce::AudioParameterBool*, kNumResonators> polarity_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> mix_volume_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> matrix_reflections_{};

    juce::AudioParameterBool* midi_drive_{};
    juce::AudioParameterBool* allow_round_robin_{};
    juce::AudioParameterFloat* global_pitch_{};
    juce::AudioParameterFloat* global_damp_{};
    juce::AudioParameterFloat* dry_mix_{};

    bool was_midi_drive_{false};
    PolyphonyManager note_manager_;

    Resonator dsp_;
private:
    void ProcessCommon(juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void ProcessMidi(juce::AudioBuffer<float>&, juce::MidiBuffer&);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorAudioProcessor)
};
