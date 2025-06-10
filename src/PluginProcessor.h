#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/filter.hpp"
#include "dsp/lpc.hpp"
#include "dsp/pitch_shifter.hpp"

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
    struct {
        struct FloatStore : public juce::AudioProcessorParameter::Listener {
            std::function<void(float)> func;
            juce::AudioParameterFloat* ptr;

            FloatStore(std::function<void(float)> func, juce::AudioParameterFloat* ptr) : func(func), ptr(ptr) {
                ptr->addListener(this);
            }
            void parameterValueChanged (int parameterIndex, float newValue) override {
                func(ptr->get());
            }
            void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
        };
        struct BoolStore : public juce::AudioProcessorParameter::Listener {
            std::function<void(bool)> func;
            juce::AudioParameterBool* ptr;

            BoolStore(std::function<void(bool)> func, juce::AudioParameterBool* ptr) : func(func), ptr(ptr) {
                ptr->addListener(this);
            }
            void parameterValueChanged (int parameterIndex, float newValue) override {
                func(ptr->get());
            }
            void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
        };
        struct IntStore : public juce::AudioProcessorParameter::Listener {
            std::function<void(int)> func;
            juce::AudioParameterInt* ptr;

            IntStore(std::function<void(int)> func, juce::AudioParameterInt* ptr) : func(func), ptr(ptr) {
                ptr->addListener(this);
            }
            void parameterValueChanged (int parameterIndex, float newValue) override {
                func(ptr->get());
            }
            void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
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
    } paramListeners_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;

    juce::AudioParameterFloat* filter_pitch_;
    juce::AudioParameterFloat* filter_gain_;
    juce::AudioParameterFloat* filter_s_;
    juce::AudioParameterFloat* lpc_pitch_;
    juce::AudioParameterFloat* lpc_detune_;

    dsp::Filter filter_;
    dsp::Filter hpfilter_;
    dsp::PitchShifter shifter_;
    dsp::BackLPC lpc_;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
