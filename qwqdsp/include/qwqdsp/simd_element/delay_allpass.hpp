#pragma once
#include "qwqdsp/simd_element/delay_line.hpp"

namespace qwqdsp_simd_element {
template<class SimdType, DelayLineInterp kInterp>
class DelayAllpass {
public:
    void Init(float fs, float max_ms) {
        delay_.Init(fs, max_ms);
    }

    void Init(size_t max_samples) {
        delay_.Init(max_samples);
    }

    void Reset() noexcept {
        delay_.Reset();
    }

    SimdType Tick(SimdType const& x, float delay, float gain) noexcept {
        SimdType wd = delay_.GetBeforePush(delay);
        SimdType w = x + gain * wd;
        SimdType y = -gain * w + wd;
        delay_.Push(w);
        return y;
    }

    SimdType Tick(SimdType const& x, SimdType const& delay, float gain) noexcept {
        SimdType wd = delay_.GetBeforePush(delay);
        SimdType w = x + gain * wd;
        SimdType y = -gain * w + wd;
        delay_.Push(w);
        return y;
    }

    DelayLine<SimdType, kInterp> delay_;
};
}
