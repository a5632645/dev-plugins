#pragma once
#include "qwqdsp/osciilor/table_sine_osc.hpp"

namespace qwqdsp::oscillor {
class BLIT {
public:
    double Tick() noexcept {
        w_osc_.Tick();
        double const cosv = w_osc_.Cosine();
        double const cosnv = w_osc_.GetNPhaseCpx(n_).real();
        double const cosvnp1v = w_osc_.GetNPhaseCpx(n_ + 1).real();
        double const down = 1.0 + a0_ * a0_ - 2.0 * a0_ * cosv;
        double const up = -a0_ * cosv + 1.0 + a_ * (a0_ * cosnv - cosvnp1v);
        double const t = up / down - 1.0;
        return t * g_;
    }

    void SetW(double w) noexcept {
        w_osc_.SetFreq(w);
    }

    void SetAmp(double a) noexcept {
        a_ = a;
        a0_ = std::pow(a, 1.0 / (n_ + 1.0));
    }
    
    void SetN(size_t n) noexcept {
        n_ = n;
        g_ = 1.0f / n;
        a0_ = std::pow(a_, 1.0f / (n_ + 1.0));
    }
private:
    TableSineOsc<16> w_osc_;
    size_t n_{};
    double a0_{};
    double a_{};
    double g_{};
};
}