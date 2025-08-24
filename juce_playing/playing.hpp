#pragma once
#include <cstddef>
#include <span>
#include "juce_audio_processors/juce_audio_processors.h"
#include "qwqdsp/osciilor/dsf_special.hpp"

struct ParamListeners;

namespace dsp {
class Playing {
public:
    void Init(float fs) {
        fs_ = fs;
    }

    void GetParams(ParamListeners& p, juce::AudioProcessorValueTreeState::ParameterLayout& avpts);

    void Process(std::span<float> left, std::span<float> right) {
        const size_t len = left.size();
        for (size_t i = 0; i < len; ++i) {
            auto s = dsf_.Tick();
            left[i] = s.real() * g_ * g2_;
            right[i] = s.imag() * g_ * g2_;
        }
    }
private:
    float fs_{};
    float g_{};
    float g2_{};
    qwqdsp::oscillor::DSFSpecial dsf_;
};
}