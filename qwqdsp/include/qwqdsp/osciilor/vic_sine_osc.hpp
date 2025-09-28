#pragma once
#include <cmath>
#include <numbers>
#include <complex>

namespace qwqdsp::oscillor {

/**
 * @brief Vicanek/Levine正交振荡器，等振幅正交输出，衰减慢
 * @ref https://vicanek.de/articles/QuadOsc.pdf
 */
class VicSineOsc {
public:
    void Reset() noexcept {
        u_ = 1.0f;
        v_ = 0.0f;
    }

    void Reset(float phase) noexcept {
        u_ = std::cos(phase);
        v_ = std::sin(phase);
    }

    void KeepAmp() noexcept {
        float g = 1.0f / std::sqrt(u_ * u_ + v_ * v_);
        u_ *= g;
        v_ *= g;
    }

    float Tick() noexcept {
        float w = u_ - k1_ * v_;
        v_ = v_ + k2_ * w;
        u_ = w - k1_ * v_;
        return v_;
    }

    float Cosine() const noexcept {
        return u_;
    }

    float Sine() const noexcept {
        return v_;
    }

    void SetFreq(float f, float fs) noexcept {
        auto omega = f / fs * std::numbers::pi_v<float> * 2.0f;
        k1_ = std::tan(omega / 2.0f);
        k2_ = 2 * k1_ / (1 + k1_ * k1_);
    }

    void SetFreq(float w) noexcept {
        k1_ = std::tan(w / 2.0f);
        k2_ = 2 * k1_ / (1 + k1_ * k1_);
    }

    std::complex<float> GetCpx() const noexcept {
        return {u_, v_};
    }
private:
    float k1_{};
    float k2_{};
    float u_{1.0f};
    float v_{};
};

}