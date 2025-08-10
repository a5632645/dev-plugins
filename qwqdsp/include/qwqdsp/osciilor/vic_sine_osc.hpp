#pragma once
#include <cmath>
#include <numbers>
#include <complex>

namespace qwqdsp {

/**
 * @brief Vicanek/Levine正交振荡器，等振幅正交输出，衰减慢
 * @ref https://vicanek.de/articles/QuadOsc.pdf
 */
class VicSineOsc {
public:
    void Reset(float phase) {
        // u_ = 1.0f;
        // v_ = 0.0f;
        u_ = std::cos(phase);
        v_ = std::sin(phase);
    }

    float Tick() {
        float w = u_ - k1_ * v_;
        v_ = v_ + k2_ * w;
        u_ = w - k1_ * v_;
        return v_;
    }

    float Cosine() const {
        return u_;
    }

    void SetFreq(float f, float fs) {
        auto omega = f / fs * std::numbers::pi_v<float> * 2.0f;
        k1_ = std::tan(omega / 2.0f);
        k2_ = 2 * k1_ / (1 + k1_ * k1_);
    }

    std::complex<float> GetCpx() const {
        return {u_, v_};
    }
private:
    float k1_{};
    float k2_{};
    float u_{};
    float v_{};
};

}