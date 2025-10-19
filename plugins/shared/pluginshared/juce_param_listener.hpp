#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

struct JuceParamListener{
    struct FloatStore : public juce::AudioProcessorParameter::Listener {
        std::function<void(float)> func;
        juce::AudioParameterFloat* ptr;

        FloatStore(std::function<void(float)> _func, juce::AudioParameterFloat* _ptr) : func(_func), ptr(_ptr) {
            ptr->addListener(this);
        }
        ~FloatStore() override {
            ptr->removeListener(this);
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

        BoolStore(std::function<void(bool)> _func, juce::AudioParameterBool* _ptr) : func(_func), ptr(_ptr) {
            ptr->addListener(this);
        }
        ~BoolStore() override {
            ptr->removeListener(this);
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

        IntStore(std::function<void(int)> _func, juce::AudioParameterInt* _ptr) : func(_func), ptr(_ptr) {
            ptr->addListener(this);
        }
        ~IntStore() override {
            ptr->removeListener(this);
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

        ChoiceStore(std::function<void(int)> _func, juce::AudioParameterChoice* _ptr) : func(_func), ptr(_ptr) {
            ptr->addListener(this);
        }
        ~ChoiceStore() override {
            ptr->removeListener(this);
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