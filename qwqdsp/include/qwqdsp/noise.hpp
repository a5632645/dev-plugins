#pragma once
#include <algorithm>
#include <cstdlib>
#include "interpolation.hpp"

namespace qwqdsp {

class Noise {
public:
    void Init(float fs) noexcept {
        fs_ = fs;
    }
    void SetRate(float rate) noexcept {
        inc_ = rate / fs_;
    }
    void Reset() {
        a_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        b_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        c_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        d_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        phase_ = 0.0f;
    }
    inline float Tick() {
        phase_ += inc_;
        [[unlikely]]
        if (phase_ > 1.0f) {
            phase_ -= 1.0f;
            a_ = b_;
            b_ = c_;
            c_ = d_;
            d_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        }
        float v = Interpolation::CatmullRomSpline(a_, b_, c_, d_, phase_);
        return std::clamp(v, 0.0f, 1.0f);
    }
private:
    float fs_;
    float inc_;
    float phase_;
    float a_;
    float b_;
    float c_;
    float d_;
};

}