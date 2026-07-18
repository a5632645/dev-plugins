#pragma once
#include <simd_detector.h>
#include <memory>
#include <string_view>
#include "pluginshared/simd/inst.hpp"

namespace warpcore {

enum class FreqDistrbution {
    k0_n,
    k1_n,
    k0_2n,
    k1_2n,
};

struct Param {
    int bands{50};
    float f_high{24000.0f};
    float filter_scale{1.0f};
    int filter_order{2};
    float pitch_shift{0.0f};
    float drywet{1.0f};
    bool pitch_affect{false};
    bool fill_gap{false};
    FreqDistrbution freq_distribution{FreqDistrbution::k0_2n};
};

class Idsp {
public:
    virtual ~Idsp() = default;

    virtual void Init(float fs) = 0;
    virtual void Reset() = 0;
    virtual void Update(const warpcore::Param& p) = 0;
    virtual void Process(float* left, float* right, int num_samples) = 0;
    virtual std::string_view InstName() = 0;
};

using DspHanle = std::unique_ptr<Idsp>;

template <simd::Inst inst>
std::unique_ptr<Idsp> CreateDspImpl();

std::unique_ptr<Idsp> CreateDsp();

} // namespace warpcore
