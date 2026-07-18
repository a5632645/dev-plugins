#pragma once
#include <memory>
#include <string_view>
#include "pluginshared/simd/inst.hpp"

namespace vital_reverb {

struct Param {
    float chorus_amount{0.05f};
    float chorus_freq{0.25f};
    float wet{0.25f};
    float pre_lowpass{0.0f};
    float pre_highpass{110.0f};
    float low_damp_pitch{0.0f};
    float high_damp_pitch{90.0f};
    float low_damp_db{0.0f};
    float high_damp_db{-1.0f};
    float size{0.5f};
    float decay_ms{1000.0f};
    float pre_delay{0.0f};
    bool freeze{false};
};

class Idsp {
public:
    virtual ~Idsp() = default;

    virtual void Init(float fs) = 0;
    virtual void Reset() = 0;
    virtual void Panic() = 0;
    virtual void Update(const Param& p) = 0;
    virtual void Process(float* left, float* right, int num_samples) = 0;
    virtual std::string_view InstName() = 0;
};

using DspHanle = std::unique_ptr<Idsp>;

template <simd::Inst inst>
std::unique_ptr<Idsp> CreateDspImpl();

std::unique_ptr<Idsp> CreateDsp();

} // namespace vital_reverb
