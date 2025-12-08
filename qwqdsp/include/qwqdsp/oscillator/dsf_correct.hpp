#pragma once
#include <complex>
#include "qwqdsp/oscillator/table_sine_v3.hpp"

namespace qwqdsp_oscillator {
/**
 * @ref https://www.verklagekasper.de/synths/dsfsynthesis/dsfsynthesis.html
 * @ref https://ccrma.stanford.edu/~stilti/papers/blit.pdf
 */
template<size_t kLookupTableFracBits = 10>
class DSFCorrect {
public:
    void Reset() noexcept {
        w0_phase_ = 0;
        w_phase_ = 0;
    }

    float Tick() noexcept {
        w_phase_ += w_inc_;
        w0_phase_ += w0_inc_;

        float const sinu = sine_lut_.Sine(w0_phase_);
        float const cosu = sine_lut_.Cosine(w0_phase_);
        float const sinv = sine_lut_.Sine(w_phase_);
        float const cosv = sine_lut_.Cosine(w_phase_);
        uint32_t v_nsub1_phase = (n_ - 1) * w_phase_;
        float const cosv_nsub1 = sine_lut_.Cosine(v_nsub1_phase);
        float const sinv_nsub1 = sine_lut_.Sine(v_nsub1_phase);
        uint32_t v_n = n_ * w_phase_;
        float const cosv_n = sine_lut_.Cosine(v_n);
        float const sinv_n = sine_lut_.Sine(v_n);

        float const up1 = -a_ * (cosv * cosu + sinv * sinu);
        float const up2 = cosu;
        float const up3 = a_pow_n_ * (
            a_ * (cosu * cosv_nsub1 - sinu * sinv_nsub1)
            - (cosu * cosv_n - sinu * sinv_n)
        );
        float const down = 1.0f + a_ * a_ - 2.0f * a_ * cosv;
        return (up1 + up2 + up3) / down;
    }

    /**
     * @param w0 0~pi
     */
    void SetW0(float w0) noexcept {
        w0_inc_ = sine_lut_.Omega2PhaseInc(w0);
        w0_ = w0;
        CheckAlasing();
    }

    /**
     * @param w 0~pi
     */
    void SetWSpace(float w) noexcept {
        w_inc_ = sine_lut_.Omega2PhaseInc(w);
        w_ = w;
        CheckAlasing();
    }

    /**
     * @param n >0
     */
    void SetN(uint32_t n) noexcept {
        set_n_ = n;
        CheckAlasing();
    }

    /**
     * @param a anything
     */
    void SetAmpFactor(float a) noexcept {
        if (a <= 1.0f && a >= 1.0f - 1e-3f) {
            a = 1.0f - 1e-3f;
        }
        else if (a >= 1.0f && a <= 1.0f + 1e-3f) {
            a = 1.0f + 1e-3f;
        }
        a_ = a;
        UpdateA();
    }

    float NormalizeGain() const noexcept {
        return (1.0f - std::abs(a_)) / (1.0f - std::abs(a_pow_n_));
    }
private:
    void CheckAlasing() noexcept {
        if (w_ == 0.0f && n_ != set_n_) {
            n_ = set_n_;
            UpdateA();
        }
        else {
            uint32_t max_n = static_cast<uint32_t>((std::numbers::pi_v<float> - w0_) / w_);
            uint32_t newn = std::min(max_n, set_n_);
            if (n_ != newn) {
                n_ = newn;
                UpdateA();
            }
        }
    }

    void UpdateA() noexcept {
        a_pow_n_ = std::pow(a_, static_cast<float>(n_));
    }

    qwqdsp_oscillator::TableSineV3<float, kLookupTableFracBits> sine_lut_;
    uint32_t w0_phase_{};
    uint32_t w_phase_{};
    uint32_t w0_inc_{};
    uint32_t w_inc_{};

    uint32_t n_{};
    uint32_t set_n_{};
    float a_{};
    float a_pow_n_{};
    float w0_{};
    float w_{};
};

/**
 * @ref https://ccrma.stanford.edu/~stilti/papers/blit.pdf
 * @tparam kFlipDown
 *           true: sum(w^n * sin(u + nv))
 *          false: sum(w^n * cos(u + nv))
 */
template<bool kFlipDown, size_t kLookupTableFracBits = 10>
class DSFCorrectComplex {
public:
    void Reset() noexcept {
        w0_osc_phase_ = 0;
        w_osc_phase_ = 0;
    }
    
    float Tick() noexcept {
        w_osc_phase_ += w_osc_inc_;
        w0_osc_phase_ += w0_osc_inc_;

        float const sinu = sine_lut_.Sine(w0_osc_phase_);
        float const cosu = sine_lut_.Cosine(w0_osc_phase_);
        float const sinv = sine_lut_.Sine(w_osc_phase_);
        float const cosv = sine_lut_.Cosine(w_osc_phase_);
        uint32_t v_nsub1_phase = (n_ - 1) * w_osc_phase_;
        float const cosv_nsub1 = sine_lut_.Cosine(v_nsub1_phase);
        float const sinv_nsub1 = sine_lut_.Sine(v_nsub1_phase);
        uint32_t v_n = n_ * w_osc_phase_;
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
            return std::imag((up1 + up2 + up3) / down);
        }
        else {
            auto const up1 = -a_ * (cosv * cosu + sinv * sinu);
            auto const up2 = cosu;
            auto const up3 = a_pow_n_ * (
                a_ * (cosu * cosv_nsub1 - sinu * sinv_nsub1)
                - (cosu * cosv_n - sinu * sinv_n)
            );
            auto const down = 1.0f + a_ * a_ - 2.0f * a_ * cosv;
            return std::real((up1 + up2 + up3) / down);
        }
    }

    /**
     * @param w0 0~pi
     */
    void SetW0(float w0) noexcept {
        w0_osc_inc_ = sine_lut_.Omega2PhaseInc(w0);
        w0_ = w0;
        CheckAlasing();
    }

    /**
     * @param w 0~pi
     */
    void SetWSpace(float w) noexcept {
        w_osc_inc_ = sine_lut_.Omega2PhaseInc(w);
        w_ = w;
        CheckAlasing();
    }

    /**
     * @param n >0
     */
    void SetN(uint32_t n) noexcept {
        set_n_ = n;
        CheckAlasing();
    }

    /**
     * @param a anything
     */
    void SetAmpFactor(float a, float phase) noexcept {
        if (a <= 1.0f && a >= 1.0f - 1e-3f) {
            a = 1.0f - 1e-3f;
        }
        else if (a >= 1.0f && a <= 1.0f + 1e-3f) {
            a = 1.0f + 1e-3f;
        }
        a_ = std::polar(a, phase);
        UpdateA();
    }

    float NormalizeGain() const noexcept {
        return (1.0f - std::abs(a_)) / (1.0f - std::abs(a_pow_n_));
    }
private:
    void CheckAlasing() noexcept {
        if (w_ == 0.0f && n_ != set_n_) {
            n_ = set_n_;
            UpdateA();
        }
        else {
            uint32_t max_n = static_cast<uint32_t>((std::numbers::pi_v<float> - w0_) / w_);
            uint32_t newn = std::min(max_n, set_n_);
            if (n_ != newn) {
                n_ = newn;
                UpdateA();
            }
        }
    }

    void UpdateA() noexcept {
        a_pow_n_ = std::pow(a_, static_cast<float>(n_));
    }

    qwqdsp_oscillator::TableSineV3<float, kLookupTableFracBits> sine_lut_;
    uint32_t w_osc_phase_{};
    uint32_t w_osc_inc_{};
    uint32_t w0_osc_phase_{};
    uint32_t w0_osc_inc_{};

    uint32_t n_{};
    uint32_t set_n_{};
    std::complex<float> a_{};
    std::complex<float> a_pow_n_{};
    float w0_{};
    float w_{};
};
}
