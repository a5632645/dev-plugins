#pragma once
#include <complex>
#include <cstddef>
#include <numbers>
#include "qwqdsp/osciilor/table_sine_osc.hpp"

namespace qwqdsp::oscillor {
/**
 * s(t) = exp(jw0t) + a * exp(jw0t + jwt) + a^2 * exp(jw0t + j2wt) +...+ a^(n-1) * exp(jw0t + j(n-1)wt)
 *          exp(jw0t) * (1 - a^n * exp(jwnt))
 *      = --------------------------------------
 *               1 - a * exp(jwt)
 */
template<size_t kLookupTableFrac = 13>
class DSFClassic {
public:
    void Reset() noexcept {
        w_osc_.Reset();
        w0_osc_.Reset();
    }

    std::complex<float> Tick() noexcept {
        w_osc_.Tick();
        w0_osc_.Tick();
        auto up = w0_osc_.GetCpx() * (1.0f - a_pow_n_ * w_osc_.GetNPhaseCpx(n_));
        auto down = 1.0f - a_ * w_osc_.GetCpx();
        return up / down;
    }

    /**
     * @param w0 0~pi
     */
    void SetW0(float w0) noexcept {
        w0_osc_.SetFreq(w0);
        w0_ = w0;
        CheckAlasing();
    }

    /**
     * @param w 0~pi
     */
    void SetWSpace(float w) noexcept {
        w_osc_.SetFreq(w);
        w_ = w;
        CheckAlasing();
    }

    /**
     * @param n >0
     */
    void SetN(size_t n) noexcept {
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
            size_t max_n = static_cast<size_t>((std::numbers::pi_v<float> - w0_) / w_);
            size_t newn = std::min(max_n, set_n_);
            if (n_ != newn) {
                n_ = newn;
                UpdateA();
            }
        }
    }

    void UpdateA() noexcept {
        a_pow_n_ = std::pow(a_, static_cast<float>(n_));
    }

    TableSineOsc<kLookupTableFrac> w0_osc_{};
    TableSineOsc<kLookupTableFrac> w_osc_{};
    float w_{};
    float w0_{};
    float a_{};
    float a_pow_n_{};
    size_t n_{};
    size_t set_n_{};
};

/**
 * a = exp(a + bi)
 * s(t) = exp(jw0t) + a * exp(jw0t + jwt) + a^2 * exp(jw0t + j2wt) +...+ a^(n-1) * exp(jw0t + j(n-1)wt)
 *          exp(jw0t) * (1 - a^n * exp(jwnt))
 *      = --------------------------------------
 *               1 - a * exp(jwt)
 */
template<size_t kLookupTableFrac = 13>
class DSFComplexFactor {
public:
    void Reset() noexcept {
        w_osc_.Reset();
        w0_osc_.Reset();
    }
    
    std::complex<float> Tick() noexcept {
        w_osc_.Tick();
        w0_osc_.Tick();
        auto up = w0_osc_.GetCpx() * (1.0f - a_pow_n_ * w_osc_.GetNPhaseCpx(n_));
        auto down = 1.0f - a_ * w_osc_.GetCpx();
        return up / down;
    }

    void SetW0(float w0) noexcept {
        w0_osc_.SetFreq(w0);
        w0_ = w0;
        CheckAlasing();
    }

    void SetWSpace(float w) noexcept {
        w_osc_.SetFreq(w);
        w_ = w;
        CheckAlasing();
    }

    void SetN(size_t n) noexcept {
        set_n_ = n;
        CheckAlasing();
    }

    void SetAmpGain(float a) noexcept {
        if (a <= 1.0f && a >= 1.0f - 1e-3f) {
            a = 1.0f - 1e-3f;
        }
        else if (a >= 1.0f && a <= 1.0f + 1e-3f) {
            a = 1.0f + 1e-3f;
        }
        factor_gain_ = a;
        auto s = std::polar(a, factor_phase_);
        a_ = s;
        UpdateA();
    }

    void SetAmpPhase(float phase) noexcept {
        factor_phase_ = phase;
        auto s = std::polar(factor_gain_, factor_phase_);
        a_ = s;
        UpdateA();
    }

    float NormalizeGain() const noexcept {
        float a = std::abs(a_);
        float apown = std::abs(a_pow_n_);
        return (1.0f - a) / (1.0f - apown);
    }
private:
    void CheckAlasing() noexcept {
        if (w_ == 0.0f && n_ != set_n_) {
            n_ = set_n_;
            UpdateA();
        }
        else {
            size_t max_n = (std::numbers::pi_v<float> - w0_) / w_;
            size_t newn = std::min(max_n, set_n_);
            if (n_ != newn) {
                n_ = newn;
                UpdateA();
            }
        }
    }

    void UpdateA() noexcept {
        a_pow_n_ = std::pow(a_, static_cast<float>(n_));
    }

    TableSineOsc<kLookupTableFrac> w0_osc_{};
    TableSineOsc<kLookupTableFrac> w_osc_{};
    float w_{};
    float w0_{};
    float factor_gain_{};
    float factor_phase_{};
    std::complex<float> a_{};
    std::complex<float> a_pow_n_{};
    size_t n_{};
    size_t set_n_{};
};
}