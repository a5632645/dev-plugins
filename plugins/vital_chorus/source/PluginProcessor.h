#pragma once
#include <pluginshared/preset_manager.hpp>

#include "vital_chorus.hpp"


// ---------------------------------------- juce processor ----------------------------------------

enum class LFOTempoType {
    Free = 0,
    Sync,
    SyncDot,
    SyncTri,
    NumTypes
};

class VitalChorusAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    VitalChorusAudioProcessor();
    ~VitalChorusAudioProcessor() override;

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

    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;
    std::unique_ptr<pluginshared::PresetManager> preset_manager_;

    juce::AudioParameterFloat* param_freq_;
    juce::AudioParameterFloat* param_depth_;
    juce::AudioParameterFloat* param_delay1_;
    juce::AudioParameterFloat* param_delay2_;
    juce::AudioParameterFloat* param_feedback_;
    juce::AudioParameterFloat* param_mix_;
    juce::AudioParameterFloat* param_cutoff_;
    juce::AudioParameterFloat* param_spread_;
    juce::AudioParameterFloat* param_num_voices_;
    juce::AudioParameterChoice* param_tempo_idx_;
    juce::AudioParameterInt* param_sync_type_;

    VitalChorus dsp_;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VitalChorusAudioProcessor)
};
