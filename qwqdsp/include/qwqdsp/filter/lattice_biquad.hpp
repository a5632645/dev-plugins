#pragma once
#include "qwqdsp/filter/biquad_coeff.hpp"

namespace qwqdsp_filter {
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
        /* defined by book */
        // float t = x - k2_ * ss2_;
        // float w2 = ss2_ + k2_ * t;
        // float w0 = t - k1_ * ss1_;
        // float w1 = ss1_ + k1_ * w0;
        // ss2_ = w1;
        // ss1_ = w0;
        // return v2_ * w2 + v1_ * w1 + v0_ * w0;

        /* optimise for speed */
        float y = v2_ * s2_ + v1_ * s1_ + v0_ * x;
        float t = x - k2_ * s2_;
        float w0 = t - k1_ * s1_;
        float w1 = s1_ + k1_ * w0;
        s2_ = w1;
        s1_ = w0;
        return y;
    }

    void Set(float b0, float b1, float b2, float a1, float a2) noexcept {
        float k2 = a2;
        float k1 = a1 / (1 + a2);
        k2_ = k2;
        k1_ = k1;
        /* defined by book */
        // float v2 = b2;
        // float v1 = b1 - a1 * b2;
        // float v0 = b0 - k1 * b1 + (a1 * k1 - a2) * b2;
        
        /* optimise for speed */
        // v0_ = v0 + k1 * v1 + k2 * v2;
        // v1_ = v1 - k1 * v0 - k1 * k1 * v1;
        // v2_ = v2 - k2 * v0 - k2 * k2 * v2 - k1 * k2 * v1;

        /* easier ladder coefficients */
        v0_ = b0;
        v1_ = (b1 - a1 * b0 - a1 * b2 + a2 * b1) / (a2 + 1);
        v2_ = b2 - a2 * b0;
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
