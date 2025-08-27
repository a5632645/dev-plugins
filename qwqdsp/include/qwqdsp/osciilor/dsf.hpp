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
    std::complex<float> Tick() {
        w_osc_.Tick();
        w0_osc_.Tick();
        auto up = w0_osc_.GetCpx() * (1.0f - a_pow_n_ * w_osc_.GetNPhaseCpx(n_));
        auto down = 1.0f - a_ * w_osc_.GetCpx();
        return up / down;
    }

    /**
     * @param w0 0~pi
     */
    void SetW0(float w0) {
        w0_osc_.SetFreq(w0);
        w0_ = w0;
        CheckAlasing();
    }

    /**
     * @param w 0~pi
     */
    void SetWSpace(float w) {
        w_osc_.SetFreq(w);
        w_ = w;
        CheckAlasing();
    }

    /**
     * @param n >0
     */
    void SetN(size_t n) {
        set_n_ = n;
        CheckAlasing();
    }

    /**
     * @param a anything
     */
    void SetAmpFactor(float a) {
        if (a <= 1 && a >= 1 - 1e-3) {
            a = 1 - 1e-3f;
        }
        else if (a >= 1 && a <= 1 + 1e-3f) {
            a = 1 + 1e-3f;
        }
        a_ = a;
        UpdateA();
    }

    float NormalizeGain() const {
        return (1.0f - a_) / (1.0f - a_pow_n_);
    }
private:
    void CheckAlasing() {
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

    void UpdateA() {
        a_pow_n_ = std::pow(a_, n_);
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
    std::complex<float> Tick() {
        w_osc_.Tick();
        w0_osc_.Tick();
        auto up = w0_osc_.GetCpx() * (1.0f - a_pow_n_ * w_osc_.GetNPhaseCpx(n_));
        auto down = 1.0f - a_ * w_osc_.GetCpx();
        return up / down;
    }

    void SetW0(float w0) {
        w0_osc_.SetFreq(w0);
        w0_ = w0;
        CheckAlasing();
    }

    void SetWSpace(float w) {
        w_osc_.SetFreq(w);
        w_ = w;
        CheckAlasing();
    }

    void SetN(size_t n) {
        set_n_ = n;
        CheckAlasing();
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
        UpdateA();
    }

    void SetAmpPhase(float phase) {
        factor_phase_ = phase;
        auto s = std::polar(factor_gain_, factor_phase_);
        a_ = s;
        UpdateA();
    }

    float NormalizeGain() const {
        float a = std::abs(a_);
        float apown = std::abs(a_pow_n_);
        return (1.0f - a) / (1.0f - apown);
    }
private:
    void CheckAlasing() {
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

    void UpdateA() {
        a_pow_n_ = std::pow(a_, n_);
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

/**
 * 这是一个特殊的DSF计算，不幸的是它总会出现意外之外的click导致无法使用
 * wolfram alpha:
 *      sum cos(km) * exp(j(w0 + wk)t), k from 0 to n
 * 它产生一个来自加法合成器才能做到的频谱
 * 第一个分音位于w0,之后间隔w一个分音，振幅呈现cos形状有一种梳状滤波器的感觉
 */
// class DSFSpecial {
// public:
//     std::complex<float> Tick() {
//         w_osc_.Tick();
//         w0_osc_.Tick();
//         auto up1 = m1_ * w_osc_.GetNPhaseCpx(n_ + 1)
//             + m2_ * w_osc_.GetCpx()
//             + m3_ * w_osc_.GetCpx()
//             - m4_ * w_osc_.GetNPhaseCpx(n_ + 2)
//             - m5_ * w_osc_.GetNPhaseCpx(n_ + 2)
//             - m6_
//             + w_osc_.GetNPhaseCpx(n_ + 1);
//         auto up = up1 * m7_ * w0_osc_.GetCpx();
//         auto down = (m8_ - w_osc_.GetCpx()) * (m8_ * w_osc_.GetCpx() - 1.0f) * 2.0f;
//         auto g = std::abs(down);
//         return up / down;
//     }

//     void SetW0(float w0) {
//         w0_osc_.SetFreq(w0);
//     }

//     void SetWSpace(float w) {
//         w_osc_.SetFreq(w);
//         w_ = w;
//     }

//     void SetN(size_t n) {
//         n_ = n;
//         Update();
//     }

//     void SetM(float m) {
//         m_ = m;
//         Update();
//     }

//     float NormalizeGain() const {
//         return 1;
//     }
// private:
//     void Update() {
//         m1_ = std::polar(1.0f, (n_ + 1) * 2 * m_);
//         m2_ = std::polar(1.0f, m_ * n_);
//         m3_ = std::polar(1.0f, m_ * (n_ + 2));
//         m4_ = std::polar(1.0f, m_);
//         m5_ = std::polar(1.0f, 2 * m_ * n_ + m_);
//         m6_ = std::polar(2.0f, m_ * (n_ + 1));
//         m7_ = std::polar(1.0f, -m_ * n_);
//         m8_ = std::polar(1.0f, m_);
//     }

//     TableSineOsc<16> w0_osc_{};
//     TableSineOsc<16> w_osc_{};
//     float w_{};
//     size_t n_{};
//     float m_{};

//     std::complex<float> last_out_{};

//     std::complex<float> m1_{};
//     std::complex<float> m2_{};
//     std::complex<float> m3_{};
//     std::complex<float> m4_{};
//     std::complex<float> m5_{};
//     std::complex<float> m6_{};
//     std::complex<float> m7_{};
//     std::complex<float> m8_{};
// };
}