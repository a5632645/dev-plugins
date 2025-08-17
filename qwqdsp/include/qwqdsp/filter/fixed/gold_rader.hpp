#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <cmath>
#include "acc_traits.hpp"

// TODO: add tempalte
//       fix coeffience calculation
//       add quantization noise reduce feedback
namespace qwqdsp::filter::fixed {
class GoldRader {
public:
    using QTYPE = int16_t;
    using ACCType = AccType<QTYPE>;
    static constexpr ACCType kMax = std::numeric_limits<QTYPE>::max();
    static constexpr ACCType kMin = std::numeric_limits<QTYPE>::min();
    static constexpr QTYPE FRAC_LEN = 15;

    QTYPE Tick(QTYPE x) {
        ACCType acc = b0_ * x + b1_ * x1_ + b2_ * x2_;
        x2_ = x1_;
        x1_ = x;
        acc += y1_ * rcos_ - y2_ * rsin_;
        ACCType temp = y2_ * rcos_ + rsin_ * y1_;
        y2_ = std::clamp(temp >> FRAC_LEN, kMin, kMax);
        y1_ = std::clamp(acc >> FRAC_LEN, kMin, kMax);
        return std::clamp(temp >> (FRAC_LEN - bshift_), kMin, kMax);
    }

    void MakeFromFloat(float b0, float b1, float b2, float pole_radius, float pole_phase) {
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
    ACCType mask_{};
};
}