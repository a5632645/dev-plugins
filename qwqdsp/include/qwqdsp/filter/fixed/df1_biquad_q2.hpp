#pragma once
#include <cstddef>
#include <cmath>
#include <limits>
#include "acc_traits.hpp"

namespace qwqdsp::filter::fixed {
template<class QTYPE, size_t FRAC_LEN>
class DF1_Biquad2 {
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
        quantization2_ = 0;
    }

    QTYPE Tick(QTYPE x) noexcept {
        ACCType acc = quantization_ * 2 - quantization2_;
        acc += (ACCType)(b0_ * x);
        acc += (ACCType)(b1_ * x1_);
        acc += (ACCType)(b2_ * x2_);
        acc -= (ACCType)(a1_ * y1_);
        acc -= (ACCType)(a2_ * y2_);

        ACCType temp = acc >> shift_;
        if (temp > kMax) temp = kMax;
        else if (temp < kMin) temp = kMin;

        quantization2_ = quantization_;
        quantization_ = acc & mask_;

        x2_ = x1_;
        x1_ = x;
        y2_ = y1_;
        y1_ = (QTYPE)temp;
        return y1_;
    }

    void MakeFromFloat(float b0, float b1, float b2, float a1, float a2) noexcept {
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
        if (std::abs(a1) > maxb) {
            maxb = std::abs(a1);
        }
        if (std::abs(a2) > maxb) {
            maxb = std::abs(a2);
        }
        int xshift = 0;
        while (maxb >= 1.0f) {
            maxb /= 2.0f;
            xshift++;
        }

        shift_ = FRAC_LEN - xshift;
        b0_ = (QTYPE)((ACCType)(b0 * (ACCType(1) << FRAC_LEN)) >> xshift);
        b1_ = (QTYPE)((ACCType)(b1 * (ACCType(1) << FRAC_LEN)) >> xshift);
        b2_ = (QTYPE)((ACCType)(b2 * (ACCType(1) << FRAC_LEN)) >> xshift);
        a1_ = (QTYPE)((ACCType)(a1 * (ACCType(1) << FRAC_LEN)) >> xshift);
        a2_ = (QTYPE)((ACCType)(a2 * (ACCType(1) << FRAC_LEN)) >> xshift);
        mask_ = (1 << (FRAC_LEN - xshift)) - 1;
    }
private:
    QTYPE x1_;
    QTYPE x2_;
    QTYPE y1_;
    QTYPE y2_;
    QTYPE b0_;
    QTYPE b1_;
    QTYPE b2_;
    QTYPE a1_;
    QTYPE a2_;
    AccType<QTYPE> quantization_;
    AccType<QTYPE> quantization2_;
    AccType<QTYPE> mask_;
    QTYPE shift_;
};
}