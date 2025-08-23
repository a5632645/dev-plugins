#pragma once
#include <complex>
#include <cstddef>
#include "qwqdsp/osciilor/table_sine_osc.hpp"

namespace qwqdsp::oscillor {
/**
 * s(t) = exp(jw0t) + a * exp(jw0t + jwt) + a^2 * exp(jw0t + j2wt) +...+ a^(n-1) * exp(jw0t + j(n-1)wt)
 *          exp(jw0t) * (1 - a^n * exp(jwnt))
 *      = --------------------------------------
 *               1 - a * exp(jwt)
 */
class DSFClassic {
public:
    std::complex<float> Tick() {
        w_osc_.Tick();
        w0_osc_.Tick();
        auto up = w0_osc_.GetCpx() * (1.0f - a_pow_n_ * w_osc_.GetNPhaseCpx(n_));
        auto down = 1.0f - a_ * w_osc_.GetCpx();
        return up / down;
    }

    void SetW0(float w0) {
        w0_osc_.SetFreq(w0);
    }

    void SetWSpace(float w) {
        w_osc_.SetFreq(w);
        w_ = w;
    }

    void SetN(size_t n) {
        n_ = n;
        a_pow_n_ = std::pow(a_, n);
    }

    void SetAmpFactor(float a) {
        if (a <= 1 && a >= 1 - 1e-5) {
            a = 1 - 1e-5f;
        }
        else if (a >= 1 && a <= 1 + 1e-5f) {
            a = 1 + 1e-5f;
        }
        a_ = a;
        a_pow_n_ = std::pow(a, n_);
    }

    float NormalizeGain() const {
        return (1.0f - a_) / (1.0f - a_pow_n_);
    }
private:
    TableSineOsc<13> w0_osc_{};
    TableSineOsc<13> w_osc_{};
    float w_{};
    float a_{};
    float a_pow_n_{};
    size_t n_{};
};

/**
 * a = exp(a + bi)
 * s(t) = exp(jw0t) + a * exp(jw0t + jwt) + a^2 * exp(jw0t + j2wt) +...+ a^(n-1) * exp(jw0t + j(n-1)wt)
 *          exp(jw0t) * (1 - a^n * exp(jwnt))
 *      = --------------------------------------
 *               1 - a * exp(jwt)
 */
class DSFComplexFactor {
public:
    std::complex<float> Tick() {
        w_osc_.Tick();
        w0_osc_.Tick();
        auto up = w0_osc_.GetCpx() * (1.0f - a_pow_n_ * w_osc_.GetNPhaseCpx(n_));
        auto down = 1.0f - a_ * w_osc_.GetCpx();
        return up / down;
    }

    void SetW0(float w0) {
        w0_osc_.SetFreq(w0);
    }

    void SetWSpace(float w) {
        w_osc_.SetFreq(w);
        w_ = w;
    }

    void SetN(size_t n) {
        n_ = n;
        a_pow_n_ = std::pow(a_, n);
    }

    void SetAmpGain(float a) {
        if (a <= 1 && a >= 1 - 1e-6) {
            a = 1 - 1e-6f;
        }
        else if (a >= 1 && a <= 1 + 1e-6f) {
            a = 1 + 1e-6f;
        }
        factor_gain_ = a;
        auto s = std::polar(a, factor_phase_);
        a_ = s;
        a_pow_n_ = std::pow(s, n_);
    }

    void SetAmpPhase(float phase) {
        factor_phase_ = phase;
        auto s = std::polar(factor_gain_, factor_phase_);
        a_ = s;
        a_pow_n_ = std::pow(s, n_);
    }

    float NormalizeGain() const {
        float a = std::abs(a_);
        float apown = std::abs(a_pow_n_);
        return (1.0f - a) / (1.0f - apown);
    }
private:
    TableSineOsc<13> w0_osc_{};
    TableSineOsc<13> w_osc_{};
    float w_{};
    float factor_gain_{};
    float factor_phase_{};
    std::complex<float> a_{};
    std::complex<float> a_pow_n_{};
    size_t n_{};
};
}