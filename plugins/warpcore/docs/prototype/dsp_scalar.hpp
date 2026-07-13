#pragma once
#include <cassert>
#include <cmath>
#include <complex>
#include <numbers>

#include "warpcore.hpp"

namespace warpcore {
struct SvfData {
    float g[IWarpCore::kMaxPoles];
    float d[IWarpCore::kMaxPoles];

    alignas(32) std::complex<float> s1[IWarpCore::kMaxPoles][IWarpCore::kMaxBands];
    alignas(32) std::complex<float> s2[IWarpCore::kMaxPoles][IWarpCore::kMaxBands];

    void Reset() noexcept {
        std::fill(&s1[0][0], &s1[0][0] + (IWarpCore::kMaxPoles * IWarpCore::kMaxBands), 0.0f);
        std::fill(&s2[0][0], &s2[0][0] + (IWarpCore::kMaxPoles * IWarpCore::kMaxBands), 0.0f);
    }

    void SetPole(int pole_idx, float w, float Q) noexcept {
        float r2 = 1.0f / Q;
        float g_val = std::tan(w / 2.0f);
        g[pole_idx] = g_val;
        d[pole_idx] = 1.0f / (1.0f + r2 * g_val + g_val * g_val);
    }

    void SetFreq(float w_lowpass, int num_filters) noexcept {
        int n = 2 * num_filters;
        for (int k = 1; k <= num_filters; ++k) {
            float phi =
                (2.0f * static_cast<float>(k) - 1.0f) * std::numbers::pi_v<float> / (2.0f * static_cast<float>(n));
            float Q = 1.0f / (2.0f * std::sin(phi));
            SetPole(k - 1, w_lowpass, Q);
        }
    }
};

class WarpCoreScalar final : public IWarpCore {
public:
    ~WarpCoreScalar() override = default;

    void Init(float fs) override {
        fs_ = fs;
    }

    void Reset() noexcept override {
        pre_osc_phase_ = 0;
        post_osc_phase_ = 0;
        svf_data_.Reset();
    }

    void Process(float* left, float* right, int num_samples) noexcept override {
        if (warp_first_) {
            ProcessPoles<true>(left, right, num_samples);
        }
        else {
            ProcessPoles<false>(left, right, num_samples);
        }
    }

    void Update(const IWarpCore::Param& p) noexcept override {
        num_warps_ = p.bands;
        warp_first_ = p.warp_first;

        float finc = p.f_high / static_cast<float>(p.bands);
        float fbase = finc / 2;
        osc_base_freq_ = fbase;
        pre_osc_phase_inc_ = fbase / fs_;
        post_osc_phase_inc_ = pre_osc_phase_inc_ * p.post_osc_freq_mul;

        if (p.post_osc_freq_mul == 1.0f) {
            post_osc_phase_ = pre_osc_phase_;
            post_osc_phase_inc_ = pre_osc_phase_inc_;
        }

        // butterworth lowpass
        float wbase = Freq2Omega(fbase);
        float filter_w = wbase * p.filter_scale;
        filter_w = std::min(filter_w, std::numbers::pi_v<float> / 2 - 1e-5f);

        poles_ = p.filter_order;
        svf_data_.SetFreq(filter_w, poles_);
    }

    float Freq2Omega(float f) noexcept {
        return 2 * std::numbers::pi_v<float> * f / fs_;
    }
private:
    template <bool kWarpFirst>
    void ProcessPoles(float* left, float* right, int num_samples) noexcept {
        switch (poles_) {
            case 1:
                ProcessInternal<1, kWarpFirst>(left, right, num_samples);
                break;
            case 2:
                ProcessInternal<2, kWarpFirst>(left, right, num_samples);
                break;
            case 3:
                ProcessInternal<3, kWarpFirst>(left, right, num_samples);
                break;
            case 4:
                ProcessInternal<4, kWarpFirst>(left, right, num_samples);
                break;
            default:
                assert(false);
                break;
        }
    }

    template <int kPoles, bool kWarpFirst>
    void ProcessInternal(float* left, float* right, int num_samples) noexcept {
        for (int i = 0; i < num_samples; i++) {
            pre_osc_phase_ += pre_osc_phase_inc_;
            pre_osc_phase_ -= std::floor(pre_osc_phase_);

            post_osc_phase_ += post_osc_phase_inc_;
            post_osc_phase_ -= std::floor(post_osc_phase_);

            constexpr float twopi = 2.0f * std::numbers::pi_v<float>;
            std::complex<float> pre_osc = {std::cos(pre_osc_phase_ * twopi), std::sin(pre_osc_phase_ * twopi)};
            std::complex<float> post_osc = {std::cos(post_osc_phase_ * twopi), std::sin(post_osc_phase_ * twopi)};
            std::complex<float> pre_osc_n_val = pre_osc;
            std::complex<float> post_osc_n_val = post_osc;

            float x = left[i];
            float y = 0.0f;

            int band_begin_idx = 0;
            if constexpr (!kWarpFirst) {
                band_begin_idx = 1;

                std::complex<float> cpx_x = x * pre_osc;
                #pragma unroll
                for (int k = 0; k < kPoles; ++k) {
                    const float gk = svf_data_.g[k];
                    const float dk = svf_data_.d[k];
                    auto& s1jk = svf_data_.s1[k][0];
                    auto& s2jk = svf_data_.s2[k][0];

                    auto bp = dk * (gk * (cpx_x - s2jk) + s1jk);
                    auto v1 = bp - s1jk;
                    auto v2 = gk * bp;
                    auto lp = v2 + s2jk;
                    
                    s1jk = bp + v1;
                    s2jk = lp + v2;
                    cpx_x = lp;
                }
                y += (cpx_x * std::conj(post_osc)).real();

                pre_osc_n_val *= pre_osc;
                post_osc_n_val *= post_osc;
            }

            for (int j = band_begin_idx; j < num_warps_; ++j) {
                std::complex<float> tmp = x * pre_osc_n_val;
                pre_osc_n_val *= pre_osc;

                #pragma unroll
                for (int k = 0; k < kPoles; ++k) {
                    const float gk = svf_data_.g[k];
                    const float dk = svf_data_.d[k];
                    auto& s1jk = svf_data_.s1[k][j];
                    auto& s2jk = svf_data_.s2[k][j];

                    auto bp = dk * (gk * (tmp - s2jk) + s1jk);
                    auto v1 = bp - s1jk;
                    auto v2 = gk * bp;
                    auto lp = v2 + s2jk;
                    
                    s1jk = bp + v1;
                    s2jk = lp + v2;
                    tmp = lp;
                }

                y += (tmp * post_osc_n_val).real();
                post_osc_n_val *= post_osc;
            }

            left[i] = y;
            right[i] = y;
        }
    }

    float fs_{};
    int num_warps_{};
    int poles_{};
    bool warp_first_{};

    // complex sine generator
    float osc_base_freq_{};

    float pre_osc_phase_{};
    float pre_osc_phase_inc_{};

    float post_osc_phase_{};
    float post_osc_phase_inc_{};

    // complex lowpass svf
    SvfData svf_data_;
};
} // namespace warpcore
