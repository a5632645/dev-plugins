#pragma once
#include "qwqdsp/filter/biquad_coeff.hpp"

namespace qwqdsp_filter {
/**
 * @brief TDF2
 *       b0 + b1*z^-1 + b2*z^-2
 * H(z)=------------------------
 *        1 + a1*z^-1 + a2*z^-2
 */
class Biquad {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    float Tick(float x) noexcept {
        auto output = x * b0_ + s1_;
        s1_ = x * b1_ - output * a1_ + s2_;
        s2_ = x * b2_ - output * a2_;
        return output;
    }

    void Set(float b0, float b1, float b2, float a1, float a2) noexcept {
        b0_ = b0;
        b1_ = b1;
        b2_ = b2;
        a1_ = a1;
        a2_ = a2;
    }

    void Set(BiquadCoeff const& c) noexcept {
        b0_ = c.b0;
        b1_ = c.b1;
        b2_ = c.b2;
        a1_ = c.a1;
        a2_ = c.a2;
    }

    void Copy(const Biquad& other) noexcept {
        b0_ = other.b0_;
        b1_ = other.b1_;
        b2_ = other.b2_;
        a1_ = other.a1_;
        a2_ = other.a2_;
    }
private:
    float b0_{};
    float b1_{};
    float b2_{};
    float a1_{};
    float a2_{};
    float s1_{};
    float s2_{};
};
}
