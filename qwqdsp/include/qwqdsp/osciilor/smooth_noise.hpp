#pragma once
#include "qwqdsp/osciilor/noise.hpp"
#include "qwqdsp/interpolation.hpp"

namespace qwqdsp::oscillor {
class SmoothNoise {
public:
    WhiteNoise& GetNoise() noexcept {
        return noise_;
    }

    void Reset() {
        a_ = noise_.Next();
        b_ = noise_.Next();
        c_ = noise_.Next();
        d_ = noise_.Next();
        phase_ = 0.0f;
    }

    void SetRate(float f, float fs) noexcept {
        inc_ = f / fs;
    }

    inline float Tick() {
        phase_ += inc_;
        if (phase_ > 1.0f) {
            phase_ -= 1.0f;
            a_ = b_;
            b_ = c_;
            c_ = d_;
            d_ = noise_.Next();
        }
        return Interpolation::PCHIP(a_, b_, c_, d_, phase_);
    }
private:
    float inc_{};
    float phase_{};
    float a_{};
    float b_{};
    float c_{};
    float d_{};
    WhiteNoise noise_;
};
}