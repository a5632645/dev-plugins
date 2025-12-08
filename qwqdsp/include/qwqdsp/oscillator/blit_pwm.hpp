#pragma once
#include <complex>
#include "table_sine_v3.hpp"
#include "qwqdsp/misc/integrator.hpp"

namespace qwqdsp_oscillator {
class BlitPWM {
public:
    float Impluse() noexcept {
        return TickOdd() * g_;
    }

    float PWM() noexcept {
        return pwm_g_ * pwm_inte_.Tick(TickOdd());
    }

    void SetW(float w) noexcept {
        v_inc_ = table_.Omega2PhaseInc(w);
        float const n_max = (std::numbers::pi_v<float> / w);
        max_n_ = static_cast<uint32_t>(std::floor(n_max));
        UpdateN();
        UpdateA();
        pwm_g_ = pwm_inte_.Gain(w);
    }

    void SetAmp(float amp) noexcept {
        a_ = amp;
        UpdateA();
    }

    void SetPWM(float width) noexcept {
        phase_ = width * std::numbers::pi_v<float>;
        cp_a0_ = std::polar(a0_, phase_);
        cp_a_ = std::polar(a_, phase_ * (n_ + 1.0f));
    }

    void SetNLimit(uint32_t n) noexcept {
        n_limit_ = n;
        UpdateN();
        UpdateA();
    }
private:
    void UpdateA() noexcept {
        a0_ = std::pow(a_, 1.0 / (n_ + 1.0));
        g_ = (1.0 - a0_) / (1.0 - a_);
        cp_a0_ = std::polar(a0_, phase_);
        cp_a_ = std::polar(a_, phase_ * (n_ + 1.0f));
    }

    void UpdateN() noexcept {
        n_ = std::min(n_limit_, max_n_);
    }

    float TickOdd() noexcept {
        v_phase_ += v_inc_;
        auto const up = cp_a0_ * table_.Sine(v_phase_)
                        + cp_a_ * (cp_a0_ * table_.Sine( n_ * v_phase_) - table_.Sine((n_ + 1) * v_phase_));
        auto const down = 1.0f + cp_a0_ * cp_a0_ - 2.0f * cp_a0_ * table_.Cosine(v_phase_);
        return std::imag(up / down);
    }

    TableSineV3<float> table_;
    uint32_t v_inc_{};
    uint32_t v_phase_{};
    uint32_t n_{1};
    uint32_t n_limit_{1};
    uint32_t max_n_{1};
    float a0_{};
    float a_{};
    std::complex<float> cp_a0_{};
    std::complex<float> cp_a_{};
    float g_{};
    float phase_{};

    qwqdsp_misc::IntegratorTrapezoidalLeak<float> pwm_inte_;
    float pwm_g_{};
};
}
