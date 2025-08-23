#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <functional>
#include "juce_core/juce_core.h"
#include "qwqdsp/performance.hpp"
#include "playing.hpp"

struct ParamListeners {
    juce::AudioProcessor& p_;

    ParamListeners(juce::AudioProcessor& p) : p_(p) {}

    struct FloatStore : public juce::AudioProcessorParameter::Listener {
        std::function<void(float)> func;
        juce::AudioParameterFloat* ptr;
        juce::AudioProcessor& p_;

        FloatStore(std::function<void(float)> func, juce::AudioParameterFloat* ptr, juce::AudioProcessor& p)
            : func(func)
            , ptr(ptr)
            , p_(p)
        {
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
        juce::AudioProcessor& p_;

        BoolStore(std::function<void(bool)> func, juce::AudioParameterBool* ptr, juce::AudioProcessor& p)
            : func(func)
            , ptr(ptr)
            , p_(p)
        {
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
        juce::AudioProcessor& p_;

        IntStore(std::function<void(int)> func, juce::AudioParameterInt* ptr, juce::AudioProcessor& p)
            : func(func)
            , ptr(ptr)
            , p_(p)
        {
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
        juce::AudioProcessor& p_;

        ChoiceStore(std::function<void(int)> func, juce::AudioParameterChoice* ptr, juce::AudioProcessor& p)
            : func(func)
            , ptr(ptr)
            , p_(p)
        {
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
        listeners.emplace_back(std::make_unique<FloatStore>(func, p.get(), p_));
    }
    void Add(const std::unique_ptr<juce::AudioParameterBool>& p, std::function<void(bool)> func) {
        listeners.emplace_back(std::make_unique<BoolStore>(func, p.get(), p_));
    }
    void Add(const std::unique_ptr<juce::AudioParameterInt>& p, std::function<void(int)> func) {
        listeners.emplace_back(std::make_unique<IntStore>(func, p.get(), p_));
    }
    void Add(const std::unique_ptr<juce::AudioParameterChoice>& p, std::function<void(int)> func) {
        listeners.emplace_back(std::make_unique<ChoiceStore>(func, p.get(), p_));
    }
};

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
    
    ParamListeners paramListeners_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;

    dsp::Playing dsp_;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
