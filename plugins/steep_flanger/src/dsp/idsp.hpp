#pragma once
#include <juce_core/juce_core.h>
#include <memory>
#include <string_view>
#include "pluginshared/simd/inst.hpp"

namespace juce {
class AudioPlayHead;
}

namespace steep_flanger {

// Params 的完整定义在 params.hpp，此处仅前置声明以避免循环包含
class Params;

class Idsp {
public:
    virtual ~Idsp() = default;

    virtual void Init(float fs) = 0;
    virtual void Reset() = 0;
    // 快照参数 + bpm 同步 LFO 相位/频率
    virtual void Update(Params& p, juce::AudioPlayHead* playhead) = 0;
    virtual void Process(float* left, float* right, int num_samples) = 0;
    virtual std::string_view InstName() = 0;
    virtual void GetCoeffs(float* out, int n) = 0;
    virtual bool ExchangeNewCoeff() = 0;
};

using DspHanle = std::unique_ptr<Idsp>;

template <simd::Inst inst>
std::unique_ptr<Idsp> CreateDspImpl();

std::unique_ptr<Idsp> CreateDsp();

} // namespace steep_flanger
