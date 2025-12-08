#pragma once
#include <complex>

namespace qwqdsp_filter {
struct BiquadCoeff {
    float b0{};
    float b1{};
    float b2{};
    float a1{};
    float a2{};

    std::complex<float> DigitalResonpoce(std::complex<float> z) const noexcept {
        auto z2 = z * z;
        return (b0*z2+b1*z+b2)/(z2+a1*z+a2);
    }
};
inline static constexpr BiquadCoeff kBiquadPassthrough{1,0,0,0,0};

struct DoubleBiquadCoeff {
    double b0{};
    double b1{};
    double b2{};
    double a1{};
    double a2{};

    BiquadCoeff ToFloat() const noexcept {
        return BiquadCoeff{
            static_cast<float>(b0),
            static_cast<float>(b1),
            static_cast<float>(b2),
            static_cast<float>(a1),
            static_cast<float>(a2)
        };
    }
};
}
