#pragma once
#include <complex>
#include <qwqdsp/oscillator/table_sine_v3.hpp>

namespace analogsynth {
class Osc4 {
public:
    void Reset(float phase) {
        w_phase_ = 0;
        w0_phase_ = 0;
        std::ignore = phase;
    }

    void Update() noexcept {
        w0_inc_ = sine_lut_.Omega2PhaseInc(w0);
        w_inc_ = sine_lut_.Omega2PhaseInc(w);
        a_ = std::polar(a, width);
        int max_n = static_cast<int>((std::numbers::pi_v<float> - w0) / w);
        max_n = std::max<int>(max_n, 1);
        uint32_t uint_max_n = static_cast<uint32_t>(max_n);
        use_n_ = use_max_n ? uint_max_n : std::min(uint_max_n, n);
        a_pow_n_ = std::pow(a_, static_cast<float>(use_n_));
        norm_gain_ = (1.0f - std::abs(a_)) / (1.0f - std::abs(a_pow_n_));
    }

    template<bool kFlipDown>
    float Tick() noexcept {
        w_phase_ += w_inc_;
        w0_phase_ += w0_inc_;

        float const sinu = sine_lut_.Sine(w0_phase_);
        float const cosu = sine_lut_.Cosine(w0_phase_);
        float const sinv = sine_lut_.Sine(w_phase_);
        float const cosv = sine_lut_.Cosine(w_phase_);
        uint32_t v_nsub1_phase = (use_n_ - 1) * w_phase_;
        float const cosv_nsub1 = sine_lut_.Cosine(v_nsub1_phase);
        float const sinv_nsub1 = sine_lut_.Sine(v_nsub1_phase);
        uint32_t v_n = use_n_ * w_phase_;
        float const cosv_n = sine_lut_.Cosine(v_n);
        float const sinv_n = sine_lut_.Sine(v_n);

        if constexpr (kFlipDown) {
            auto const up1 = a_ * (sinv * cosu - cosv * sinu);
            auto const up2 = sinu;
            auto const up3 = a_pow_n_ * (
                a_ * (sinu * cosv_nsub1 + cosu * sinv_nsub1)
                - (sinu * cosv_n + cosu * sinv_n)
            );
            auto const down = 1.0f + a_ * a_ - 2.0f * a_ * cosv;
            return std::imag((up1 + up2 + up3) / down) * norm_gain_;
        }
        else {
            auto const up1 = -a_ * (cosv * cosu + sinv * sinu);
            auto const up2 = cosu;
            auto const up3 = a_pow_n_ * (
                a_ * (cosu * cosv_nsub1 - sinu * sinv_nsub1)
                - (cosu * cosv_n - sinu * sinv_n)
            );
            auto const down = 1.0f + a_ * a_ - 2.0f * a_ * cosv;
            return std::real((up1 + up2 + up3) / down) * norm_gain_;
        }
    }

    float a{};
    float width{};
    uint32_t n{};
    float w0{};
    float w{};
    bool use_max_n{};
private:
    qwqdsp_oscillator::TableSineV3<float> sine_lut_;
    std::complex<float> a_{};
    std::complex<float> a_pow_n_{};
    uint32_t w_phase_{};
    uint32_t w_inc_{};
    uint32_t w0_phase_{};
    uint32_t w0_inc_{};
    uint32_t use_n_{};
    float norm_gain_{};
};
}
