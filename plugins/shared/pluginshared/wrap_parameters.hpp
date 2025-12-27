#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <pluginshared/bpm_sync_lfo.hpp>

namespace pluginshared {
// -------------------- warpper of juce parameters --------------------
class FloatParam {
public:
    FloatParam(juce::StringRef name, juce::NormalisableRange<float> range, float default_value)
        : name_(name)
        , range_(range)
        , default_value_(default_value) {
    }

    std::unique_ptr<juce::AudioParameterFloat> Build() {
        jassert(ptr_ == nullptr);
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{name_, 1},
            name_,
            range_,
            default_value_
        );
        ptr_ = p.get();
        return p;
    }

    float Get() noexcept {
        return ptr_->get();
    }

    juce::AudioProcessorValueTreeState::ParameterLayout& operator+=(
        juce::AudioProcessorValueTreeState::ParameterLayout& layout
    ) {
        layout.add(Build());
        return layout;
    }

    juce::AudioParameterFloat* ptr_{};
    juce::String name_;
    juce::NormalisableRange<float> range_;
    float default_value_;
};

template<bool kNegPos>
class BpmSyncFreqParam {
public:
    using LFOSyncType = typename pluginshared::BpmSyncLFO<kNegPos>::LFOTempoType;

    BpmSyncFreqParam(
        juce::StringRef name,
        juce::NormalisableRange<float> range,
        float default_hz_value,
        LFOSyncType default_lfo_tempo_type,
        juce::StringRef default_lfo_tempo_speed)
        : freq_(name, range, default_hz_value)
        , default_lfo_tempo_type_(default_lfo_tempo_type)
        , default_lfo_tempo_(default_lfo_tempo_speed) {
    }

    std::unique_ptr<juce::AudioParameterFloat> Build1() {
        jassert(freq_.ptr_ == nullptr);
        auto p = freq_.Build();
        state_.param_lfo_hz_ = p.get();
        return p;
    }
    std::unique_ptr<juce::AudioParameterInt> Build2() {
        jassert(state_.param_lfo_tempo_type_ == nullptr);
        auto p = state_.MakeLfoTempoTypeParam(freq_.name_ + "_tt", default_lfo_tempo_type_);
        state_.param_lfo_tempo_type_ = p.get();
        return p;
    }
    std::unique_ptr<juce::AudioParameterChoice> Build3() {
        jassert(state_.param_tempo_speed_ == nullptr);
        auto p = state_.MakeLfoTempoSpeedParam(freq_.name_ + "_ts", default_lfo_tempo_);
        state_.param_tempo_speed_ = p.get();
        return p;
    }

    float Get() noexcept {
        return state_.GetLfoFreq();
    }

    juce::AudioProcessorValueTreeState::ParameterLayout& operator+=(
        juce::AudioProcessorValueTreeState::ParameterLayout& layout
    ) {
        layout.add(Build1());
        layout.add(Build2());
        layout.add(Build3());
        return layout;
    }

    FloatParam freq_;
    pluginshared::BpmSyncLFO<kNegPos> state_;
    LFOSyncType default_lfo_tempo_type_;
    juce::StringRef default_lfo_tempo_;
};

class ChoiceParam {
public:
    ChoiceParam(juce::StringRef name, juce::StringArray choice, juce::StringRef default_name)
        : name_(name)
        , choices_(choice)
        , default_value_(choice.indexOf(default_name)) {
        jassert(default_value_ != -1);
    }

    std::unique_ptr<juce::AudioParameterChoice> Build() {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{name_, 1},
            name_,
            choices_,
            default_value_
        );
        ptr_ = p.get();
        return p;
    }

    int Get() noexcept {
        return ptr_->getIndex();
    }

    juce::AudioProcessorValueTreeState::ParameterLayout& operator+=(
        juce::AudioProcessorValueTreeState::ParameterLayout& layout
    ) {
        layout.add(Build());
        return layout;
    }

    juce::AudioParameterChoice* ptr_{};
private:
    juce::StringRef name_;
    juce::StringArray choices_;
    int default_value_;
};

class BoolParam {
public:
    BoolParam(juce::StringRef name, bool default_value)
        : name_(name)
        , default_value_(default_value) {}

    std::unique_ptr<juce::AudioParameterBool> Build() {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{name_, 1},
            name_,
            default_value_
        );
        ptr_ = p.get();
        return p;
    }

    bool Get() noexcept {
        return ptr_->get();
    }

    juce::AudioProcessorValueTreeState::ParameterLayout& operator+=(
        juce::AudioProcessorValueTreeState::ParameterLayout& layout
    ) {
        layout.add(Build());
        return layout;
    }

    juce::AudioParameterBool* ptr_{};
private:
    juce::StringRef name_;
    bool default_value_;
};
}
