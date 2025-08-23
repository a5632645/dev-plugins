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
        nw_osc_.Tick();
        w0_osc_.Tick();
        auto up = w0_osc_.GetCpx() * (1.0f - a_pow_n_ * nw_osc_.GetCpx());
        auto down = 1.0f - a_ * w_osc_.GetCpx();
        return up / down;
    }

    void SetW0(float w0) {
        w0_osc_.SetFreq(w0);
    }

    void SetWSpace(float w) {
        w_osc_.SetFreq(w);
        nw_osc_.SetFreq(w * n_);
        w_ = w;
    }

    void SetN(size_t n) {
        n_ = n;
        a_pow_n_ = std::pow(a_, n);
        nw_osc_.SetFreq(n * w_);
    }

    void SetAmpFactor(float a) {
        if (a <= 1 && a >= 1 - 1e-6) {
            a = 1 - 1e-6f;
        }
        else if (a >= 1 && a <= 1 + 1e-6f) {
            a = 1 + 1e-6f;
        }
        a_ = a;
        a_pow_n_ = std::pow(a, n_);
    }
private:
    TableSineOsc<16> w0_osc_{};
    TableSineOsc<16> nw_osc_{};
    TableSineOsc<16> w_osc_{};
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
        nw_osc_.Tick();
        w0_osc_.Tick();
        auto up = w0_osc_.GetCpx() * (1.0f - a_pow_n_ * nw_osc_.GetCpx());
        auto down = 1.0f - a_ * w_osc_.GetCpx();
        return up / down;
    }

    void SetW0(float w0) {
        w0_osc_.SetFreq(w0);
    }

    void SetWSpace(float w) {
        w_osc_.SetFreq(w);
        nw_osc_.SetFreq(w * n_);
        w_ = w;
    }

    void SetN(size_t n) {
        n_ = n;
        a_pow_n_ = std::pow(a_, n);
        nw_osc_.SetFreq(n * w_);
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
private:
    TableSineOsc<16> w0_osc_{};
    TableSineOsc<16> nw_osc_{};
    TableSineOsc<16> w_osc_{};
    float w_{};
    float factor_gain_{};
    float factor_phase_{};
    std::complex<float> a_{};
    std::complex<float> a_pow_n_{};
    size_t n_{};
};
}