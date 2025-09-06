#pragma once
#include "qwqdsp/osciilor/table_sine_v2.hpp"
#include "qwqdsp/misc/integrator.hpp"

namespace qwqdsp::oscillor {
class Blit {
public:
    double Impluse() noexcept {
        return TickRaw() * saw_blit_g_;
    }

    double OddImpluse() noexcept {
        return TickRawOdd() * odd_blit_g_;
    }

    double Sawtooth() noexcept {
        double const it = TickRaw() * saw_g_;
        return saw_inte_.Tick(it * 2.0);
    }

    double Sqaure() noexcept {
        double const it = TickRawOdd() * square_g_;
        return square_inte_.Tick(it);
    }

    double Triangle() noexcept {
        double const it = TickRawOdd() * square_g_;
        double const square = square_inte_.Tick(it) * square_g_;
        return triangle_inte_.Tick(square);
    }

    // --------------------------------------------------------------------------------
    // set
    // --------------------------------------------------------------------------------
    /**
     * @param w [0, pi]
     */
    void SetW(double w) noexcept {
        w_ = w;
        phase_inc_ = table_.Omega2PhaseInc(w);

        saw_n_ = static_cast<uint32_t>(std::floor(std::numbers::pi_v<double> / w));
        saw_a0_ = std::pow(amp_, 1.0 / (saw_n_ + 1.0));
        saw_blit_g_ = (1.0 - saw_a0_) / (saw_a0_ * (1.0 - amp_));
        // 实际上后面的因子准确值是 1/pi
        saw_g_ = saw_inte_.Gain(w) * 0.3;
        
        odd_n_ = static_cast<uint32_t>(std::floor((std::numbers::pi_v<double> / w - 1) / 2));
        odd_a0_ = std::pow(amp_, 1.0 / (odd_n_ + 1.0));
        odd_blit_g_ = (1.0 - odd_a0_) / (1.0 - amp_);
        square_g_ = square_inte_.Gain(w);
    }

    /**
     * @param a (0, 1)
     */
    void SetAmp(double a) noexcept {
        amp_ = a;
        UpdateA();
    }
private:
    void UpdateA() noexcept {
        saw_a0_ = std::pow(amp_, 1.0 / (saw_n_ + 1.0));
        saw_blit_g_ = (1.0 - saw_a0_) / (saw_a0_ * (1.0 - amp_));
        
        odd_a0_ = std::pow(amp_, 1.0 / (odd_n_ + 1.0));
        odd_blit_g_ = (1.0 - odd_a0_) / (odd_a0_ * (1.0 - amp_));
    }

    // 原始公式sum a0^k * cos(wk), k from 0 to n,但是移除了k=0
    double TickRaw() noexcept {
        phase_ += phase_inc_;

        double const up = -saw_a0_ * table_.Cosine(phase_)
                        + 1.0
                        + amp_ * (saw_a0_ * table_.Cosine(phase_ * saw_n_) - table_.Cosine(phase_ * (saw_n_ + 1)));
        double const down = 1.0 + saw_a0_ * saw_a0_ - 2.0 * saw_a0_ * table_.Cosine(phase_);
        return up / down - 1.0;
    }

    // sum a0^k * cos(w + 2kw), k from 0 to n
    double TickRawOdd() noexcept {
        phase_ += phase_inc_;
        double const up = -odd_a0_ * table_.Cosine(phase_)
                        + table_.Cosine(phase_)
                        + amp_ * (odd_a0_ * table_.Cosine(phase_ + phase_ * 2 * odd_n_) - table_.Cosine(phase_ + 2 * phase_ * (odd_n_ + 1)));
        double const down = 1.0 + odd_a0_ * odd_a0_ - 2.0 * odd_a0_ * table_.Cosine(phase_ * 2);
        return up / down;
    }

    // blit
    TableSineV2<double> table_;
    double w_{};
    uint32_t phase_inc_{};
    uint32_t phase_{};
    double amp_{};
    
    // saw
    double saw_blit_g_{};
    double saw_a0_{};
    uint32_t saw_n_{};
    double saw_g_{};
    misc::IntegratorTrapezoidalLeak<double> saw_inte_;

    // sqaure
    double odd_blit_g_{};
    double odd_a0_{};
    uint32_t odd_n_{};
    double square_g_{};
    misc::IntegratorTrapezoidalLeak<double> square_inte_;

    // triangle
    misc::IntegratorTrapezoidalLeak<double> triangle_inte_;
};
}