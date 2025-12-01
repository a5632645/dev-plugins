#pragma once
#include "qwqdsp/filter/biquad_coeff.hpp"

namespace qwqdsp::filter {
/**
 * @brief lattice form
 *       b0 + b1*z^-1 + b2*z^-2
 * H(z)=------------------------
 *        1 + a1*z^-1 + a2*z^-2
 * @ref https://dsp.stackexchange.com/questions/48255/real-time-modulated-iir-filter
 */
class LatticeBiquad {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    float Tick(float x) noexcept {
        float t = x - k2_ * s2_;
        float w1 = s2_ + k2_ * t;
        float w3 = t - k1_ * s1_;
        float w2 = s1_ + k1_ * w3;
        s2_ = w2;
        s1_ = w3;
        return v2_ * w1 + v1_ * w2 + v0_ * w3;
    }

    void Set(float b0, float b1, float b2, float a1, float a2) noexcept {
        k2_ = a2;
        k1_ = a1 / (1 + a2);
        v2_ = b2;
        v1_ = b1 - a1 * b2;
        v0_ = b0 - k1_ * b1 + (a1 * k1_ - a2) * b2;
    }

    void Set(BiquadCoeff const& c) noexcept {
        Set(c.b0, c.b1, c.b2, c.a1, c.a2);
    }

    void Copy(const LatticeBiquad& other) noexcept {
        v0_ = other.v0_;
        v1_ = other.v1_;
        v2_ = other.v2_;
        k1_ = other.k1_;
        k2_ = other.k2_;
    }
private:
    float v0_{};
    float v1_{};
    float v2_{};
    float k1_{};
    float k2_{};
    float s1_{};
    float s2_{};
};
}