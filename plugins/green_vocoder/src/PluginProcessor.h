#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "pluginshared/juce_param_listener.hpp"
#include "pluginshared/preset_manager.hpp"

#include "dsp/engine.hpp"
#include "param_mailboxes.hpp"
#include "params.hpp"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor {
public:
    static constexpr auto kParameterValueTreeIdentify = "PARAMETERS";
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
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
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void Panic();

    JuceParamListener paramListeners_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;
    std::unique_ptr<pluginshared::PresetManager> preset_manager_;

    Params params_;
    green_vocoder::Engine engine_;

    // --- param mailboxes (message thread → audio thread) ---
    TiltFilterMailbox tilt_mb_;
    LeakyLpcMailbox leaky_lpc_mb_;
    BlockLpcMailbox block_lpc_mb_;
    STFTVocoderMailbox stft_mb_;
    ChannelVocoderMailbox cv_mb_;
    PitchOscMailbox pitch_osc_mb_;

    int old_latency_{};
private:
    static constexpr size_t kBlockSize = 256;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
