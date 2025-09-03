#pragma once
#include <algorithm>
#include "qwqdsp/osciilor/table_sine_osc.hpp"
#include "qwqdsp/misc/integrator.hpp"

namespace qwqdsp::oscillor {
class BLIT {
public:
    double Impluse() noexcept {
        return TickRaw() * g_;
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
        w_osc_.SetFreq(w);
        CheckAlasing();

        saw_g_ = saw_inte_.Gain(w) * 0.3;

        SetWOdd(w);
    }

    /**
     * @param a (0, 1)
     */
    void SetAmp(double a) noexcept {
        a_ = a;
        UpdateA();
    }
    
    /**
     * @param [1, ...]
     */
    void SetN(size_t n) noexcept {
        set_n_ = n;
        CheckAlasing();
    }
private:
    void CheckAlasing() noexcept {
        if (w_ == 0.0 && n_ != set_n_) {
            n_ = set_n_;
            UpdateA();
        }
        else {
            size_t max_n = static_cast<size_t>((std::numbers::pi_v<float>) / w_);
            size_t newn = std::min(max_n, set_n_);
            if (n_ != newn) {
                n_ = newn;
                UpdateA();
            }
        }
    }
    
    void UpdateA() noexcept {
        a0_ = std::pow(a_, 1.0 / (n_ + 1.0));
        g_ = (1.0 - a0_) / (a0_ * (1.0 - a_));
    }

    // 原始公式sum a0^k * cos(wk), k from 0 to n,但是移除了k=0
    double TickRaw() noexcept {
        w_osc_.Tick();
        double const cosv = w_osc_.Cosine();
        double const cosnv = w_osc_.GetNPhaseCpx(n_).real();
        double const cosvnp1v = w_osc_.GetNPhaseCpx(n_ + 1).real();
        double const down = 1.0 + a0_ * a0_ - 2.0 * a0_ * cosv;
        double const up = -a0_ * cosv + 1.0 + a_ * (a0_ * cosnv - cosvnp1v);
        double const t = up / down - 1.0; // 移除k=0
        return t;
    }

    void SetWOdd(double w) noexcept {
        u_inc_ = w_osc_.Omega2PhaseInc(w);
        v2_inc_ = w_osc_.Omega2PhaseInc(w * 2);
        square_g_ = square_inte_.Gain(w);
        trianlge_g_ = square_g_ * square_g_;
    }

    // sum a0^k * cos(w + 2kw), k from 0 to n
    // TODO: 现在的n计算会导致混叠
    double TickRawOdd() noexcept {
        double const up = -a0_ * w_osc_.Cosine(v2_phase_ - u_phase_)
                        + w_osc_.Cosine(u_phase_)
                        + a_ * (a0_ * w_osc_.Cosine(u_phase_ + n_ * v2_phase_) - w_osc_.Cosine(u_phase_ + (n_ + 1) * v2_phase_));
        double const down = 1.0 + a0_ * a0_ - 2.0 * a0_ * w_osc_.Cosine(v2_phase_);
        u_phase_ += u_inc_;
        v2_phase_ += v2_inc_;
        return up / down;
    }

    // blit
    TableSineOsc<16> w_osc_;
    double w_{};
    size_t set_n_{};
    size_t n_{};
    double a0_{};
    double a_{};
    double g_{};

    // saw integrator
    double saw_g_{};
    misc::IntegratorLeak<double> saw_inte_;

    // sqaure
    uint32_t u_phase_{};
    uint32_t u_inc_{};
    uint32_t v2_phase_{};
    uint32_t v2_inc_{};
    double square_g_{};
    misc::IntegratorLeak<double> square_inte_;

    // triangle
    misc::IntegratorLeak<double> triangle_inte_;
    double trianlge_g_{};
};
}