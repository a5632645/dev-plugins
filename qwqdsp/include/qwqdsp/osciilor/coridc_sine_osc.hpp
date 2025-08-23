#pragma once
#include <cmath>
#include <complex>
#include <numbers>

namespace qwqdsp::oscillor {
/**
 * @brief Cordic正交振荡器，衰减较快，等振幅正交输出
 */
class CordicSineOsc {
public:
    void Reset(float phase) {
        now_.real(std::cos(phase));
        now_.imag(std::sin(phase));
    }

    float Tick() {
        now_ *= inc_;
        return now_.imag();
    }

    void SetFreq(float f, float fs) {
        auto omega = f / fs * std::numbers::pi_v<float> * 2.0f;
        inc_.real(std::cos(omega));
        inc_.imag(std::sin(omega));
    }

    float Cosine() const {
        return now_.real();
    }

    std::complex<float> GetCpx() const {
        return now_;
    }
private:
    std::complex<float> now_;
    std::complex<float> inc_;
};
}