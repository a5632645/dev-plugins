#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "qwqdsp/fx/delay_line.hpp"
#include "qwqdsp/misc/smoother.hpp"

struct JuceParamListener{
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
};

//==============================================================================
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


    static constexpr size_t kMaxCoeffLen = 64;
    static constexpr size_t kMaxNumNotchs = 1000;
    qwqdsp::fx::DelayLine<> delay_left_;
    qwqdsp::fx::DelayLine<> delay_right_;
    qwqdsp::misc::ExpSmoother notch_smoother_;
    std::array<float, kMaxCoeffLen> coeffs_;
    size_t coeff_len_{};
    float side_lobe_{20};
    float cutoff_w_{};
    void UpdateCoeff();


private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessor)
};
