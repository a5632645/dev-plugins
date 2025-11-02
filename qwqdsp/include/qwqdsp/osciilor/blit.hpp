#pragma once
#include "qwqdsp/osciilor/table_sine_v3.hpp"
#include "qwqdsp/misc/integrator.hpp"

namespace qwqdsp::oscillor {
class Blit {
public:
    float Sine() noexcept {
        phase_ += phase_inc_;
        return table_.Sine(phase_);
    }

    float Impluse() noexcept {
        return TickRaw() * saw_blit_g_;
    }

    float OddImpluse() noexcept {
        return TickRawOdd() * odd_blit_g_;
    }

    float Sawtooth() noexcept {
        float const it = TickRaw() * saw_g_;
        return saw_inte_.Tick(it * 2.0);
    }

    float Sqaure() noexcept {
        float const it = TickRawOdd() * square_g_;
        return square_inte_.Tick(it);
    }

    float Triangle() noexcept {
        float const it = TickRawOdd() * square_g_;
        float const square = square_inte_.Tick(it) * square_g_;
        return triangle_inte_.Tick(square);
    }

    // --------------------------------------------------------------------------------
    // set
    // --------------------------------------------------------------------------------
    /**
     * @param w [0, pi]
     */
    void SetW(float w) noexcept {
        w_ = w;
        phase_inc_ = table_.Omega2PhaseInc(w);

        max_saw_n_ = static_cast<uint32_t>(std::floor(std::numbers::pi_v<float> / w));
        max_odd_n_ = static_cast<uint32_t>(std::floor((std::numbers::pi_v<float> / w - 1) / 2));
        UpdateN();
        UpdateA();
        // 后面的准确值是 1/pi
        saw_g_ = saw_inte_.Gain(w) * 0.3;
        square_g_ = square_inte_.Gain(w);
    }

    /**
     * @param a (0, 1)
     * @note 如果你使用float,在默认参数下最好不要超过0.9
     */
    void SetAmp(float a) noexcept {
        amp_ = a;
        UpdateA();
    }

    void SetNLimit(uint32_t n) noexcept {
        n_limit_ = n;
        UpdateN();
        UpdateA();
    }
private:
    void UpdateA() noexcept {
        saw_a0_ = std::pow(amp_, 1.0 / (saw_n_ + 1.0));
        saw_blit_g_ = (1.0 - saw_a0_) / (saw_a0_ * (1.0 - amp_));
        
        odd_a0_ = std::pow(amp_, 1.0 / (odd_n_ + 1.0));
        odd_blit_g_ = (1.0 - odd_a0_) / (odd_a0_ * (1.0 - amp_));
    }

    void UpdateN() noexcept {
        saw_n_ = std::min(n_limit_, max_saw_n_);
        odd_n_ = std::min(n_limit_, max_odd_n_);
    }

    // 原始公式sum a0^k * cos(wk), k from 0 to n,但是移除了k=0
    float TickRaw() noexcept {
        phase_ += phase_inc_;

        float const up = -saw_a0_ * table_.Cosine(phase_)
                        + 1.0
                        + amp_ * (saw_a0_ * table_.Cosine(phase_ * saw_n_) - table_.Cosine(phase_ * (saw_n_ + 1)));
        float const down = 1.0 + saw_a0_ * saw_a0_ - 2.0 * saw_a0_ * table_.Cosine(phase_);
        return up / down - 1.0;
    }

    // sum a0^k * cos(w + 2kw), k from 0 to n
    float TickRawOdd() noexcept {
        phase_ += phase_inc_;
        float const up = -odd_a0_ * table_.Cosine(phase_)
                        + table_.Cosine(phase_)
                        + amp_ * (odd_a0_ * table_.Cosine(phase_ + phase_ * 2 * odd_n_) - table_.Cosine(phase_ + 2 * phase_ * (odd_n_ + 1)));
        float const down = 1.0 + odd_a0_ * odd_a0_ - 2.0 * odd_a0_ * table_.Cosine(phase_ * 2);
        return up / down;
    }

    // blit
    TableSineV3<float> table_;
    float w_{};
    uint32_t phase_inc_{};
    uint32_t phase_{};
    float amp_{};
    uint32_t n_limit_{1};
    
    // saw
    float saw_blit_g_{};
    float saw_a0_{};
    uint32_t saw_n_{1};
    uint32_t max_saw_n_{1};
    float saw_g_{};
    misc::IntegratorTrapezoidalLeak<float> saw_inte_;

    // sqaure
    float odd_blit_g_{};
    float odd_a0_{};
    uint32_t odd_n_{1};
    uint32_t max_odd_n_{1};
    float square_g_{};
    misc::IntegratorTrapezoidalLeak<float> square_inte_;

    // triangle
    misc::IntegratorTrapezoidalLeak<float> triangle_inte_;
};
}