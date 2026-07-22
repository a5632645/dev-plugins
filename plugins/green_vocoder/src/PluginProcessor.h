#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "pluginshared/juce_param_listener.hpp"
#include "pluginshared/preset_manager.hpp"

#include "dsp/block_burg_lpc.hpp"
#include "dsp/channel_vocoder.hpp"
#include "dsp/leaky_burg_lpc.hpp"
#include "dsp/stft_vocoder.hpp"
#include "dsp/tilt_filter.hpp"

#include "dsp/pitch_osc.hpp"

#include "params.hpp"

#include <qwqdsp/simd_element/algebraic_waveshaper.hpp>

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
    void SetLatency();
    JuceParamListener paramListeners_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;
    std::unique_ptr<pluginshared::PresetManager> preset_manager_;

    Params params_;

    // crossing buffer
    std::array<qwqdsp_simd_element::PackFloat<2>, 256> crossing_main_buffer_;
    std::array<qwqdsp_simd_element::PackFloat<2>, 256> crossing_side_buffer_;
    // dsps
    green_vocoder::dsp::TiltFilter pre_tilt_filter_;
    green_vocoder::dsp::LeakyBurgLPC burg_lpc_;
    green_vocoder::dsp::BlockBurgLPC block_burg_lpc_;
    green_vocoder::dsp::STFTVocoder stft_vocoder_;
    green_vocoder::dsp::ChannelVocoder channel_vocoder_;
    // pitch tracking → oscillator
    green_vocoder::dsp::PitchOsc pitch_osc_;
    bool first_init_{};

    int old_latency_{};
    std::atomic<int> latency_{};

    eVocoderType last_vocoder_type_{eVocoderType_LeakyBurgLPC};
private:
    static constexpr size_t kBlockSize = 256;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
