#pragma once
#include <complex>
#include "table_sine_v2.hpp"
#include "qwqdsp/misc/integrator.hpp"

namespace qwqdsp::oscillor {
class BlitPWM {
public:
    double Impluse() noexcept {
        return TickOdd() * g_;
    }

    double PWM() noexcept {
        return pwm_g_ * pwm_inte_.Tick(TickOdd());
    }

    void SetW(double w) noexcept {
        v_inc_ = table_.Omega2PhaseInc(w);
        double const n_max = (std::numbers::pi_v<double> / w);
        n_ = static_cast<uint32_t>(std::floor(n_max));
        UpdateA();
        pwm_g_ = pwm_inte_.Gain(w);
    }

    void SetAmp(double amp) noexcept {
        a_ = amp;
        UpdateA();
    }

    void SetPWM(double width) noexcept {
        phase_ = width * std::numbers::pi_v<double>;
        cp_a0_ = std::polar(a0_, phase_);
        cp_a_ = std::polar(a_, phase_ * (n_ + 1.0));
    }
private:
    void UpdateA() noexcept {
        g_ = (1.0 - a0_) / (1.0 - a_);
        a0_ = std::pow(a_, 1.0 / (n_ + 1.0));
        cp_a0_ = std::polar(a0_, phase_);
        cp_a_ = std::polar(a_, phase_ * (n_ + 1.0));
    }

    double TickOdd() noexcept {
        v_phase_ += v_inc_;
        auto const up = cp_a0_ * table_.Sine(v_phase_)
                        + cp_a_ * (cp_a0_ * table_.Sine( n_ * v_phase_) - table_.Sine((n_ + 1) * v_phase_));
        auto const down = 1.0 + cp_a0_ * cp_a0_ - 2.0 * cp_a0_ * table_.Cosine(v_phase_);
        return std::imag(up / down);
    }

    TableSineV2<double> table_;
    uint32_t v_inc_{};
    uint32_t v_phase_{};
    uint32_t n_{};
    double a0_{};
    double a_{};
    std::complex<double> cp_a0_{};
    std::complex<double> cp_a_{};
    double g_{};
    double phase_{};

    misc::IntegratorTrapezoidalLeak<double> pwm_inte_;
    double pwm_g_{};
};
}