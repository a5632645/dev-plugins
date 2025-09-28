#pragma once
#include <cmath>
#include <complex>
#include <numbers>

namespace qwqdsp::oscillor {
/**
 * @brief 数字谐振器正交振荡器，低频不稳定，衰减慢
 */
class DROscFull {
public:
    void Reset(float freq, float fs, float phase) noexcept {
        auto omega = freq / fs * std::numbers::pi_v<float> * 2.0f;
        sin0_ = std::sin(phase);
        sin1_ = std::sin(phase + omega);
        cos0_ = std::cos(phase);
        cos1_ = std::cos(phase + omega);
        coeff_ = 2 * std::cos(omega);
    }

    void SetFreq(float freq, float fs) noexcept {
        auto omega = freq / fs * std::numbers::pi_v<float> * 2.0f;
        float f_cos = std::cos(omega);
        float f_sin = std::sin(omega);
        sin1_ = sin0_ * f_cos + cos0_ * f_sin;
        cos1_ = cos0_ * f_cos - sin0_ * f_sin;
        coeff_ = 2 * f_cos;
    }

    float Tick() noexcept {
        auto e = coeff_ * sin1_ - sin0_;
        sin0_ = sin1_;
        sin1_ = e;
        auto e_cos = coeff_ * cos1_ - cos0_;
        cos0_ = cos1_;
        cos1_ = e_cos;
        return sin0_;
    }

    float cos() const noexcept {
        return cos0_;
    }

    std::complex<float> GetCpx() const noexcept {
        return {cos0_, sin0_};
    }
private:
    float sin0_{};
    float sin1_{};
    float cos0_{};
    float cos1_{};
    float coeff_{};
};
}