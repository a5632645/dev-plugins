#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "dsp/filter.hpp"
#include "dsp/burg_lpc.hpp"
#include "dsp/pitch_shifter.hpp"
#include "dsp/rls_lpc.hpp"
#include "dsp/stft_vocoder.hpp"
#include "dsp/gain.hpp"
#include "dsp/ensemble.hpp"
#include "dsp/channel_vocoder.hpp"
#include "qwqdsp/performance.hpp"
#include "qwqdsp/segement_process.hpp"
#include "qwqdsp/pitch/yin.hpp"
#include "qwqdsp/pitch/fast_yin.hpp"
#include "qwqdsp/osciilor/raw_oscillor.hpp"
#include "qwqdsp/osciilor/noise.hpp"

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
    struct {
        struct FloatStore : public juce::AudioProcessorParameter::Listener {
            std::function<void(float)> func;
            juce::AudioParameterFloat* ptr;

            FloatStore(std::function<void(float)> func, juce::AudioParameterFloat* ptr) : func(func), ptr(ptr) {
                ptr->addListener(this);
            }
            void parameterValueChanged (int parameterIndex, float newValue) override {
                func(ptr->get());
                (void)parameterIndex;
                (void)newValue;
            }
            void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
        };
        struct BoolStore : public juce::AudioProcessorParameter::Listener {
            std::function<void(bool)> func;
            juce::AudioParameterBool* ptr;

            BoolStore(std::function<void(bool)> func, juce::AudioParameterBool* ptr) : func(func), ptr(ptr) {
                ptr->addListener(this);
            }
            void parameterValueChanged (int parameterIndex, float newValue) override {
                func(ptr->get());
                (void)parameterIndex;
                (void)newValue;
            }
            void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
        };
        struct IntStore : public juce::AudioProcessorParameter::Listener {
            std::function<void(int)> func;
            juce::AudioParameterInt* ptr;

            IntStore(std::function<void(int)> func, juce::AudioParameterInt* ptr) : func(func), ptr(ptr) {
                ptr->addListener(this);
            }
            void parameterValueChanged (int parameterIndex, float newValue) override {
                func(ptr->get());
                (void)parameterIndex;
                (void)newValue;
            }
            void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
        };
        struct ChoiceStore : public juce::AudioProcessorParameter::Listener {
            std::function<void(int)> func;
            juce::AudioParameterChoice* ptr;

            ChoiceStore(std::function<void(int)> func, juce::AudioParameterChoice* ptr) : func(func), ptr(ptr) {
                ptr->addListener(this);
            }
            void parameterValueChanged (int parameterIndex, float newValue) override {
                func(ptr->getIndex());
                (void)parameterIndex;
                (void)newValue;
            }
            void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
        };

        std::vector<std::unique_ptr<juce::AudioProcessorParameter::Listener>> listeners;
        void CallAll() {
            for (auto& l : listeners) {
                l->parameterValueChanged(0, 0);
            }
        }
        void Add(const std::unique_ptr<juce::AudioParameterFloat>& p, std::function<void(float)> func) {
            listeners.emplace_back(std::make_unique<FloatStore>(func, p.get()));
        }
        void Add(const std::unique_ptr<juce::AudioParameterBool>& p, std::function<void(bool)> func) {
            listeners.emplace_back(std::make_unique<BoolStore>(func, p.get()));
        }
        void Add(const std::unique_ptr<juce::AudioParameterInt>& p, std::function<void(int)> func) {
            listeners.emplace_back(std::make_unique<IntStore>(func, p.get()));
        }
        void Add(const std::unique_ptr<juce::AudioParameterChoice>& p, std::function<void(int)> func) {
            listeners.emplace_back(std::make_unique<ChoiceStore>(func, p.get()));
        }
    } paramListeners_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;

    juce::AudioParameterFloat* filter_pitch_;
    juce::AudioParameterFloat* filter_gain_;
    juce::AudioParameterFloat* filter_s_;
    juce::AudioParameterFloat* lpc_pitch_;
    juce::AudioParameterFloat* lpc_detune_;
    juce::AudioParameterBool* shifter_enabled_;
    juce::AudioParameterInt* main_channel_config_;
    juce::AudioParameterInt* side_channel_config_;

    dsp::Filter filter_;
    dsp::PitchShifter shifter_;
    dsp::BurgLPC burg_lpc_;
    dsp::RLSLPC rls_lpc_;
    dsp::STFTVocoder stft_vocoder_;
    dsp::ChannelVocoder channel_vocoder_;
    dsp::Ensemble ensemble_;
    qwqdsp::SegementProcess yin_process_;
    // qwqdsp::pitch::Yin yin_;
    qwqdsp::pitch::FastYin yin_;
    qwqdsp::oscillor::RawOscillor sawtooth_;
    qwqdsp::oscillor::WhiteNoise noise_;

    dsp::Gain<1> main_gain_;
    dsp::Gain<1> side_gain_;
    dsp::Gain<2> output_gain_;

    int old_latency_{};
    std::atomic<int> latency_{};

    juce::AudioParameterChoice* vocoder_type_param_{};
#ifdef __VOCODER_ENABLE_PERFORMANCE_DEBUG
    qwqdsp::Performance perf_;
#endif
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
