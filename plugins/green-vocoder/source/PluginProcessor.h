#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "pluginshared/preset_manager.hpp"
#include "pluginshared/juce_param_listener.hpp"

#include "dsp/burg_lpc.hpp"
#include "dsp/pitch_shifter.hpp"
#include "dsp/rls_lpc.hpp"
#include "dsp/stft_vocoder.hpp"
#include "dsp/channel_vocoder.hpp"
#include "dsp/tilt_filter.hpp"
#include "dsp/ensemble.hpp"
#include "dsp/gain.hpp"

#include "qwqdsp/osciilor/polyblep.hpp"
#include "qwqdsp/osciilor/noise.hpp"

#include "qwqdsp/pitch/fast_yin.hpp"
#include "qwqdsp/pitch/yin.hpp"
#include "qwqdsp/pitch/mpm.hpp"
#include "qwqdsp/pitch/helmholtz.hpp"

#include "qwqdsp/segement/analyze.hpp"
#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/filter/fast_set_iir_paralle.hpp"
#include "qwqdsp/algebraic_waveshaper.hpp"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
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
    juce::AudioParameterBool* shifter_enabled_;
    juce::AudioParameterInt* main_channel_config_;
    juce::AudioParameterInt* side_channel_config_;

    dsp::PitchShifter shifter_;
    dsp::BurgLPC burg_lpc_;
    dsp::RLSLPC rls_lpc_;
    dsp::STFTVocoder stft_vocoder_;
    dsp::ChannelVocoder channel_vocoder_;
    dsp::Ensemble ensemble_;
    dsp::TiltFilter pre_tilt_filter_;

    // pitch tracking
    qwqdsp::segement::Analyze<8192> yin_segement_;
    qwqdsp::pitch::FastYin yin_;
    std::array<float, 8192> osc_buffer_{};
    size_t osc_wpos_{};
    float osc_want_write_frac_{};
    float last_osc_mix_{};
    float last_noise_mix_{};
    
    // tracking oscillator
    qwqdsp::oscillor::PolyBlep<float, false> tracking_osc_;
    qwqdsp::oscillor::WhiteNoise noise_;
    juce::AudioParameterChoice* tracking_waveform_{};
    juce::AudioParameterFloat* tracking_noise_{};
    float frequency_mul_{};
    qwqdsp::filter::FastSetIirParalle<qwqdsp::filter::fastset_coeff::Order2_1e7> pitch_glide_;
    bool first_init_{};

    dsp::Gain<1> main_gain_;
    dsp::Gain<1> side_gain_;
    dsp::Gain<2> output_gain_;
    qwqdsp::AlgebraicWaveshaper output_drive_left_;
    qwqdsp::AlgebraicWaveshaper output_drive_right_;

    int old_latency_{};
    std::atomic<int> latency_{};

    juce::AudioParameterChoice* vocoder_type_param_{};
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
