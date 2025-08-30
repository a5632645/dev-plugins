#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include "juce_audio_processors/juce_audio_processors.h"

#include "qwqdsp/filter/allpass.hpp"

struct ParamListeners;

namespace dsp {
class Playing {
public:
    void Init(float fs) {
        
    }

    void GetParams(ParamListeners& p, juce::AudioProcessorValueTreeState::ParameterLayout& avpts);

    void Process(std::span<float> left, std::span<float> right) {
        for (auto& s : left) {
            for (auto& f : apfs) {
                s = f.Tick(s);
            }
        }
        std::copy(left.begin(), left.end(), right.begin());
    }

    std::vector<qwqdsp::filter::AllpassPolyphase> apfs;
private:
};
}