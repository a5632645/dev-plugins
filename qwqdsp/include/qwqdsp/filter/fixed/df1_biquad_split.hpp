#pragma once
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include "acc_traits.hpp"

namespace qwqdsp::filter::fixed {
template<class QTYPE, size_t FRAC_LEN>
class DF1_BiquadSplit {
public:
    using ACCType = AccType<QTYPE>;
    static constexpr ACCType kMax = std::numeric_limits<QTYPE>::max();
    static constexpr ACCType kMin = std::numeric_limits<QTYPE>::min();

    void Reset() noexcept {
        x1_ = 0;
        x2_ = 0;
        y1_ = 0;
        y2_ = 0;
        quantization_ = 0;
    }

    QTYPE Tick(QTYPE x) noexcept {
        ACCType bacc = x * b0_ + x1_ * b1_ + x2_ * b2_;
        ACCType aacc = (bacc >> 1) - a1_ * y1_ - a2_ * y2_ + quantization_;
        x2_ = x1_;
        x1_ = x;
        y2_ = y1_;
        quantization_ = aacc & mask_;
        ACCType temp = aacc >> (FRAC_LEN - 1);
        y1_ = std::clamp(temp, kMin, kMax);
        temp = aacc >> (FRAC_LEN - 1 - bshift_);
        return std::clamp(temp, kMin, kMax);
    }

    void MakeFromFloat(float b0, float b1, float b2, float a1, float a2) noexcept {
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

        a1 /= 2.0f;
        a2 /= 2.0f;
        a1_ = (QTYPE)((ACCType)(a1 * (ACCType(1) << FRAC_LEN)));
        a2_ = (QTYPE)((ACCType)(a2 * (ACCType(1) << FRAC_LEN)));

        mask_ = (1 << (FRAC_LEN - 1)) - 1;
    }
private:
    QTYPE x1_{};
    QTYPE x2_{};
    QTYPE y1_{};
    QTYPE y2_{};

    QTYPE b0_{};
    QTYPE b1_{};
    QTYPE b2_{};
    QTYPE bshift_{};

    QTYPE a1_{};
    QTYPE a2_{};

    ACCType quantization_{};
    ACCType mask_{};
};
}