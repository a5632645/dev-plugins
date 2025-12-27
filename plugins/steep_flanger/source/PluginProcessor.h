#pragma once
#include <pluginshared/juce_param_listener.hpp>
#include <pluginshared/preset_manager.hpp>
#include <pluginshared/bpm_sync_lfo.hpp>

#include "steep_flanger.hpp"

// ---------------------------------------- juce processor ----------------------------------------
class SteepFlangerAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    SteepFlangerAudioProcessor();
    ~SteepFlangerAudioProcessor() override;

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

    juce::AudioParameterFloat* param_delay_ms_;
    juce::AudioParameterFloat* param_delay_depth_ms_;
    juce::AudioParameterFloat* param_lfo_phase_;
    juce::AudioParameterFloat* param_fir_cutoff_;
    juce::AudioParameterFloat* param_fir_coeff_len_;
    juce::AudioParameterFloat* param_fir_side_lobe_;
    juce::AudioParameterBool* param_fir_min_phase_;
    juce::AudioParameterBool* param_fir_highpass_;
    juce::AudioParameterFloat* param_feedback_;
    juce::AudioParameterFloat* param_damp_pitch_;
    juce::AudioParameterFloat* param_barber_phase_;
    juce::AudioParameterFloat* param_barber_stereo_;
    juce::AudioParameterFloat* param_drywet_;
    juce::AudioParameterBool* param_barber_enable_;
    
    SteepFlanger dsp_;
    SteepFlangerParameter dsp_param_;

    pluginshared::BpmSyncLFO<false> delay_lfo_state_;
    pluginshared::BpmSyncLFO<true> barber_lfo_state_;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessor)
};
