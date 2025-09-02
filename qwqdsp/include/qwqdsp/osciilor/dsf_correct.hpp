#pragma once
#include "qwqdsp/osciilor/table_sine_osc.hpp"

namespace qwqdsp::oscillor {
/**
 * @ref https://www.verklagekasper.de/synths/dsfsynthesis/dsfsynthesis.html
 * @ref https://ccrma.stanford.edu/~stilti/papers/blit.pdf
 */
template<size_t kLookupTableFracBits = 13>
class DSFCorrect {
public:
    void Reset() noexcept {
        w_osc_.Reset();
        w0_osc_.Reset();
    }

    float Tick() noexcept {
        float const sinu = w0_osc_.Tick();
        float const cosu = w0_osc_.Cosine();
        float const sinv = w_osc_.Tick();
        float const cosv = w_osc_.Cosine();
        auto const v_nsub1 = w_osc_.GetNPhaseCpx(n_ - 1);
        float const cosv_nsub1 = v_nsub1.real();
        float const sinv_nsub1 = v_nsub1.imag();
        auto const v_n = w_osc_.GetNPhaseCpx(n_);
        float const cosv_n = v_n.real();
        float const sinv_n = v_n.imag();

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

    oscillor::TableSineOsc<kLookupTableFracBits> w0_osc_;
    oscillor::TableSineOsc<kLookupTableFracBits> w_osc_;
    size_t n_{};
    size_t set_n_{};
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
template<bool kFlipDown, size_t kLookupTableFracBits = 13>
class DSFCorrectComplex {
public:
    void Reset() noexcept {
        w_osc_.Reset();
        w0_osc_.Reset();
    }
    
    float Tick() noexcept {
        float const sinu = w0_osc_.Tick();
        float const cosu = w0_osc_.Cosine();
        float const sinv = w_osc_.Tick();
        float const cosv = w_osc_.Cosine();
        auto const v_nsub1 = w_osc_.GetNPhaseCpx(n_ - 1);
        float const cosv_nsub1 = v_nsub1.real();
        float const sinv_nsub1 = v_nsub1.imag();
        auto const v_n = w_osc_.GetNPhaseCpx(n_);
        float const cosv_n = v_n.real();
        float const sinv_n = v_n.imag();

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

    oscillor::TableSineOsc<kLookupTableFracBits> w0_osc_;
    oscillor::TableSineOsc<kLookupTableFracBits> w_osc_;
    size_t n_{};
    size_t set_n_{};
    std::complex<float> a_{};
    std::complex<float> a_pow_n_{};
    float w0_{};
    float w_{};
};
}