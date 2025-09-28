#pragma once
#include <algorithm>
#include <limits>
#include <cmath>
#include "acc_traits.hpp"

namespace qwqdsp::filter::fixed {
/**
 * @brief 它和biquad不能互换
 *                rsin * z^-1 * (b0 + b1*z^-1 + b2*z^-2)
 * H(z) = ----------------------------------------------------------
 *         (z - 2rcos + r^2) or (z - r*exp(w))(z - conj(r*exp(w)))
 */
template<class QTYPE, size_t FRAC_LEN>
class GoldRader {
public:
    using ACCType = AccType<QTYPE>;
    static constexpr ACCType kMax = std::numeric_limits<QTYPE>::max();
    static constexpr ACCType kMin = std::numeric_limits<QTYPE>::min();
    static constexpr ACCType kMask = 1 << (FRAC_LEN - 1);

    void Reset() noexcept {
        x1_ = 0;
        x2_ = 0;
        y1_ = 0;
        y2_ = 0;
        quantization2_ = 0;
        quantization_ = 0;
    }

    QTYPE Tick(QTYPE x) noexcept {
        ACCType acc = b0_ * x + b1_ * x1_ + b2_ * x2_ + quantization_;
        x2_ = x1_;
        x1_ = x;
        acc += y1_ * rcos_;
        acc -= y2_ * rsin_;
        ACCType temp = quantization2_;
        temp += y2_ * rcos_;
        temp += rsin_ * y1_;
        quantization2_ = temp & kMask;
        y2_ = std::clamp(temp >> FRAC_LEN, kMin, kMax);
        quantization_ = acc & kMask;
        y1_ = std::clamp(acc >> FRAC_LEN, kMin, kMax);
        return std::clamp(temp >> (FRAC_LEN - bshift_), kMin, kMax);
    }

    void MakeFromFloat(float b0, float b1, float b2, float pole_radius, float pole_phase) noexcept {
        rcos_ = (QTYPE)((ACCType(1) << FRAC_LEN) * pole_radius * std::cos(pole_phase));
        rsin_ = (QTYPE)((ACCType(1) << FRAC_LEN) * pole_radius * std::sin(pole_phase));

        b0 /= rsin_;
        b1 /= rsin_;
        b2 /= rsin_;
        {
            float maxb = 0.0f;
            if (std::abs(b0) > maxb) {
                maxb = std::abs(b0);
            }
            if (std::abs(b1) > maxb) {
                maxb = std::abs(b1);
            }
            if (std::abs(b2) > maxb) {
                maxb = std::abs(b2);
            }
            bshift_ = 0;
            while (maxb >= 1.0f) {
                maxb /= 2.0f;
                ++bshift_;
            }

            b0_ = (QTYPE)((ACCType)(b0 * (ACCType(1) << FRAC_LEN)) >> bshift_);
            b1_ = (QTYPE)((ACCType)(b1 * (ACCType(1) << FRAC_LEN)) >> bshift_);
            b2_ = (QTYPE)((ACCType)(b2 * (ACCType(1) << FRAC_LEN)) >> bshift_);
        }
    }
private:
    QTYPE x1_{};
    QTYPE x2_{};
    QTYPE b0_{};
    QTYPE b1_{};
    QTYPE b2_{};
    QTYPE bshift_{};

    QTYPE y1_{};
    QTYPE y2_{};
    QTYPE rcos_{};
    QTYPE rsin_{};

    ACCType quantization_{};
    ACCType quantization2_{};
};
}