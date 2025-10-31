#pragma once
#include <cmath>

namespace qwqdsp {
/**
 * algebraic waveshaper
 *                x
 * naive = ---------------  integral = sqrt(1 + x^2)
 *          sqrt(1 + x^2)
 * @ref https://vicanek.de/articles/AADistortion.pdf
 */
class AlgebraicWaveshaper {
public:
    static inline float Naive(float x) noexcept {
        return x / std::sqrt(1 + x*x);
    }

    /**
     * @note latency = 0.5 samples
     */
    float ADAA(float x) noexcept {
        auto up = x + xn1_;
        auto F_x = std::sqrt(1 + x*x);
        auto down = F_x + F_xn1_;
        xn1_ = x;
        F_xn1_ = F_x;
        return up / down;
    }

    /**
     * @note latency = 1 samples
     */
    float ADAA_MV(float x) noexcept {
        auto F12 = std::sqrt(1 + X2(0.5f * (x + xn1_)));
        auto F1 = std::sqrt(1 + X2(xn1_));
        auto F32 = F_xn2_;
        auto y = 0.25f * ((x + 3 * xn1_) / (F12 + F1) + (3 * xn1_ + xn2_) / (F1 + F32));
        xn2_ = xn1_;
        xn1_ = x;
        F_xn2_ = F12;
        return y;
    }

    /**
     * @note latency >= 1 samples
     */
    float ADAA_MV_Compensation(float x) noexcept {
        constexpr auto kA = 0.171572875f;
        x = ADAA_MV(x);
        y_ = x + kA * (x - y_);
        return y_;
    }

    void Reset() noexcept {
        xn1_ = 0;
        xn2_ = 0;
        F_xn1_ = 1;
        F_xn2_ = 1;
        y_ = 0;
    }
private:
    static inline constexpr float X2(float x) noexcept {
        return x * x;
    }

    float xn1_{};
    float F_xn1_{1};
    float xn2_{};
    float F_xn2_{1};
    float y_{};
};
}