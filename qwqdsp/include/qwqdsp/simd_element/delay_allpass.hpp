#pragma once
#include "delay_line_single.hpp"

namespace qwqdsp_simd_element {

template<size_t N>
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

    PackFloat<N> Tick(PackFloat<N> const& x, float delay, float gain) noexcept {
        PackFloat<N> wd = delay_.GetBeforePush(delay);
        PackFloat<N> w = x + gain * wd;
        PackFloat<N> y = -gain * w + wd;
        delay_.Push(w);
        return y;
    }

    PackFloat<N> Tick(PackFloat<N> const& x, PackFloat<N> const& delay, float gain) noexcept {
        PackFloat<N> wd = delay_.GetBeforePush(delay);
        PackFloat<N> w = x + gain * wd;
        PackFloat<N> y = -gain * w + wd;
        delay_.Push(w);
        return y;
    }

    DelayLineSingle<N> delay_;
};

} // namespace qwqdsp_simd_element
