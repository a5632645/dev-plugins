#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "pluginshared/preset_manager.hpp"
#include "pluginshared/juce_param_listener.hpp"

#include "dsp/leaky_burg_lpc.hpp"
#include "dsp/stft_vocoder.hpp"
#include "dsp/channel_vocoder.hpp"
#include "dsp/tilt_filter.hpp"
#include "dsp/ensemble.hpp"
#include "dsp/mfcc_vocoder.hpp"
#include "dsp/block_burg_lpc.hpp"

#include "qwqdsp/oscillator/polyblep.hpp"
#include "qwqdsp/oscillator/noise.hpp"

#include "qwqdsp/pitch/fast_yin.hpp"

#include "qwqdsp/segement/analyze.hpp"
#include "qwqdsp/filter/fast_set_iir_paralle.hpp"
#include <qwqdsp/simd_element/algebraic_waveshaper.hpp>
#include <qwqdsp/oscillator/noise.hpp>

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr auto kParameterValueTreeIdentify = "PARAMETERS";
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

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

    void Panic();
    void SetLatency();
    JuceParamListener paramListeners_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;
    std::unique_ptr<pluginshared::PresetManager> preset_manager_;

    juce::AudioParameterFloat* lpc_pitch_;
    juce::AudioParameterFloat* lpc_detune_;
    juce::AudioParameterChoice* main_channel_config_;
    juce::AudioParameterChoice* side_channel_config_;

    // crossing buffer
    std::array<qwqdsp_simd_element::PackFloat<2>, 256> crossing_main_buffer_;
    std::array<qwqdsp_simd_element::PackFloat<2>, 256> crossing_side_buffer_;
    // dsps
    green_vocoder::dsp::TiltFilter pre_tilt_filter_;
    green_vocoder::dsp::LeakyBurgLPC burg_lpc_;
    green_vocoder::dsp::BlockBurgLPC block_burg_lpc_;
    green_vocoder::dsp::STFTVocoder stft_vocoder_;
    green_vocoder::dsp::MFCCVocoder mfcc_vocoder_;
    green_vocoder::dsp::ChannelVocoder channel_vocoder_;
    green_vocoder::dsp::Ensemble ensemble_;
    qwqdsp_oscillator::WhiteNoise noise_;

    // pitch tracking
    qwqdsp_segement::Analyze<8192> yin_segement_;
    qwqdsp_pitch::FastYin yin_;
    std::array<float, 8192> osc_buffer_{};
    size_t osc_wpos_{};
    float last_osc_mix_{};
    float last_noise_mix_{};
    
    // tracking oscillator
    qwqdsp_oscillator::PolyBlep<qwqdsp_oscillator::blep_coeff::BSpline> tracking_osc_;
    juce::AudioParameterChoice* tracking_waveform_{};
    juce::AudioParameterFloat* tracking_noise_{};
    float frequency_mul_{};
    qwqdsp_filter::FastSetIirParalle<qwqdsp_filter::fastset_coeff::Order2_1e7> pitch_glide_;
    bool first_init_{};

    juce::AudioParameterBool* output_saturation_;
    juce::AudioParameterFloat* output_drive_;
    qwqdsp_simd_element::AlgebraicWaveshaper<2> output_driver_;

    int old_latency_{};
    std::atomic<int> latency_{};

    juce::AudioParameterChoice* vocoder_type_param_{};
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
