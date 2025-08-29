#pragma once
#include <cstddef>
#include <span>
#include "juce_audio_processors/juce_audio_processors.h"

struct ParamListeners;

namespace dsp {
class Playing {
public:
    void Init(float fs) {
        
    }

    void GetParams(ParamListeners& p, juce::AudioProcessorValueTreeState::ParameterLayout& avpts);

    void Process(std::span<float> left, std::span<float> right) {
        
    }
private:
    
};
}