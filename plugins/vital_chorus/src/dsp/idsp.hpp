#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <memory>
#include <string_view>
#include "juce_audio_basics/juce_audio_basics.h"
#include "pluginshared/simd/inst.hpp"
#include "global.hpp"

namespace vital_chorus {

struct DspParam {
    float depth{};
    float delay1{};
    float delay2{};
    float feedback{};
    float mix{};
    float freq{};
    float low_w{};
    float high_w{};
    int num_voice{};
};

class Params;

class Idsp {
public:
    virtual ~Idsp() = default;

    virtual void Init(float fs) = 0;
    virtual void Reset() = 0;
    virtual void Update(const DspParam& p) = 0;
    virtual void SyncPhase(Params& p, juce::AudioPlayHead* ph) = 0;
    virtual void Process(float* __restrict left, float* __restrict right, int num_samples) = 0;
    virtual std::string_view InstName() = 0;
    // 各声道的延迟时间（ms），channel i → [i]（i 偶 = 左声道，i 奇 = 右声道），供 UI 可视化
    virtual std::array<float, global::kMaxNumChorus> GetDelayMs() const = 0;
};

using DspHandle = std::unique_ptr<Idsp>;

template <simd::Inst inst>
std::unique_ptr<Idsp> CreateDspImpl();

std::unique_ptr<Idsp> CreateDsp();

} // namespace vital_chorus
