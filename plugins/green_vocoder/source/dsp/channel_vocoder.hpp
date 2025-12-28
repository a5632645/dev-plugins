#pragma once
#include "param_ids.hpp"
#include <array>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/filter/iir_design_extra.hpp>
#include <qwqdsp/filter/iir_design.hpp>

namespace green_vocoder::dsp {
class BandSVF {
public:
    /**
     * @brief 
     * 
     * @tparam kOnlyPole true意味着你是通过Q和w计算出来的带通滤波器，false为可能携带零点的传统滤波器设计方法(qwqdsp_filter::IIRDesign)
     * @param v0_l 
     * @param v0_r 
     */
    template<bool kOnlyPole>
    void Tick(
        qwqdsp_simd_element::PackFloat<4>& v0_l,
        qwqdsp_simd_element::PackFloat<4>& v0_r
    ) noexcept {
        if constexpr (!kOnlyPole) {
            auto hp_l = (v0_l - (g_ + r2_) * s1_l_ - s2_l_) * d_;
            auto hp_r = (v0_r - (g_ + r2_) * s1_r_ - s2_r_) * d_;
            auto v1_l = g_ * hp_l;
            auto v1_r = g_ * hp_r;
            auto bp_l = v1_l + s1_l_;
            auto bp_r = v1_r + s1_r_;
            auto v2_l = g_ * bp_l;
            auto v2_r = g_ * bp_r;
            auto lp_l = v2_l + s2_l_;
            auto lp_r = v2_r + s2_r_;
            s1_l_ = bp_l + v1_l;
            s1_r_ = bp_r + v1_r;
            s2_l_ = lp_l + v2_l;
            s2_r_ = lp_r + v2_r;
            v0_l = hp_l * hp_mix_ + lp_l * lp_mix_;
            v0_r = hp_r * hp_mix_ + lp_r * lp_mix_;
        }
        else {
            // normalized bandpass
            auto bp_l = d_ * (g_ * (v0_l * r2_ - s2_l_) + s1_l_);
            auto bp_r = d_ * (g_ * (v0_r * r2_ - s2_r_) + s1_r_);
            auto bp2_l = bp_l + bp_l;
            auto bp2_r = bp_r + bp_r;
            s1_l_ = bp2_l - s1_l_;
            s1_r_ = bp2_r - s1_r_;
            auto v22_l = g_ * bp2_l;
            auto v22_r = g_ * bp2_r;
            s2_l_ += v22_l;
            s2_r_ += v22_r;
            v0_l = bp_l;
            v0_r = bp_r;
        }
    }

    void MakeBandpass(
        qwqdsp_simd_element::PackFloatCRef<4> analog_w,
        qwqdsp_simd_element::PackFloatCRef<4> Q
    ) noexcept {
        g_ = analog_w;
        r2_ = 1.0f / Q;
        d_ = 1.0f / (1.0f + r2_ * g_ + g_ * g_);
    }

    void SetAnalogPoleZero(
        std::array<qwqdsp_filter::IIRDesign::ZPK, 4> const& zpk_buffer
    ) noexcept {
        qwqdsp_simd_element::PackFloat<4> p_re{
            static_cast<float>(zpk_buffer[0].p.real()),
            static_cast<float>(zpk_buffer[1].p.real()),
            static_cast<float>(zpk_buffer[2].p.real()),
            static_cast<float>(zpk_buffer[3].p.real())
        };
        qwqdsp_simd_element::PackFloat<4> p_im{
            static_cast<float>(zpk_buffer[0].p.imag()),
            static_cast<float>(zpk_buffer[1].p.imag()),
            static_cast<float>(zpk_buffer[2].p.imag()),
            static_cast<float>(zpk_buffer[3].p.imag())
        };
        auto analog_w = qwqdsp_simd_element::PackOps::Sqrt(p_re * p_re + p_im * p_im);
        auto Q = analog_w / (-2.0f * p_re);
        Q = qwqdsp_simd_element::PackOps::Abs(Q);
        g_ = analog_w;
        r2_ = 1.0f / Q;
        d_ = 1.0f / (1.0f + r2_ * g_ + g_ * g_);

        // other channels always match this condition
        if (zpk_buffer[0].z) {
            qwqdsp_simd_element::PackFloat<4> z_re{
                static_cast<float>((*zpk_buffer[0].z).real()),
                static_cast<float>((*zpk_buffer[1].z).real()),
                static_cast<float>((*zpk_buffer[2].z).real()),
                static_cast<float>((*zpk_buffer[3].z).real())
            };
            qwqdsp_simd_element::PackFloat<4> z_im{
                static_cast<float>((*zpk_buffer[0].z).imag()),
                static_cast<float>((*zpk_buffer[1].z).imag()),
                static_cast<float>((*zpk_buffer[2].z).imag()),
                static_cast<float>((*zpk_buffer[3].z).imag())
            };
            qwqdsp_simd_element::PackFloat<4> k{
                static_cast<float>(zpk_buffer[0].k),
                static_cast<float>(zpk_buffer[1].k),
                static_cast<float>(zpk_buffer[2].k),
                static_cast<float>(zpk_buffer[3].k)
            };
            lp_mix_ = k * (z_re * z_re + z_im * z_im) / (g_ * g_);
            hp_mix_ = k;
        }
        else {
            qwqdsp_simd_element::PackFloat<4> k{
                static_cast<float>(zpk_buffer[0].k),
                static_cast<float>(zpk_buffer[1].k),
                static_cast<float>(zpk_buffer[2].k),
                static_cast<float>(zpk_buffer[3].k)
            };
            lp_mix_ = k / (g_ * g_);
            hp_mix_.Broadcast(0);
        }
    }

    void Reset() noexcept {
        s1_l_.Broadcast(0);
        s1_r_.Broadcast(0);
        s2_r_.Broadcast(0);
        s2_l_.Broadcast(0);
    }
private:
    qwqdsp_simd_element::PackFloat<4> s2_l_{};
    qwqdsp_simd_element::PackFloat<4> s2_r_{};
    qwqdsp_simd_element::PackFloat<4> s1_l_{};
    qwqdsp_simd_element::PackFloat<4> s1_r_{};
    qwqdsp_simd_element::PackFloat<4> r2_{};
    qwqdsp_simd_element::PackFloat<4> g_{};
    qwqdsp_simd_element::PackFloat<4> d_{};
    qwqdsp_simd_element::PackFloat<4> hp_mix_{};
    qwqdsp_simd_element::PackFloat<4> lp_mix_{};
};

struct CascadeBPSVF {
    template<size_t kNumFilters, bool kOnlyPole>
    void Tick(
        qwqdsp_simd_element::PackFloat<4>& l,
        qwqdsp_simd_element::PackFloat<4>& r
    ) noexcept {
        for (size_t i = 0; i < kNumFilters; ++i) {
            svf_[i].Tick<kOnlyPole>(l, r);
        }
    }

    void Reset() noexcept {
        for (auto& f : svf_) {
            f.Reset();
        }
    }

    std::array<BandSVF, 6> svf_;
};

class ChannelVocoder {
public:
    static constexpr int kMaxOrder = 100;
    static constexpr int kMinOrder = 4;

    enum class FilterBankMode {
        StackButterworth12,
        StackButterworth24,
        FlatButterworth12,
        FlatButterworth24,
        Chebyshev12,
        Chebyshev24,
        Elliptic24,
        Elliptic36,
    };

    void Init(float sample_rate, size_t block_size);
    void ProcessBlock(
        qwqdsp_simd_element::PackFloat<2>* main,
        qwqdsp_simd_element::PackFloat<2>* side,
        size_t num_samples
    );
    void PanicBiquads();

    void SetNumBands(int bands);
    void SetFreqBegin(float begin);
    void SetFreqEnd(float end);
    void SetAttack(float attack);
    void SetRelease(float release);
    void SetModulatorScale(float scale);
    void SetCarryScale(float scale);
    void SetMap(eChannelVocoderMap map);
    void SetFilterBankMode(FilterBankMode mode);
    void SetGate(float db);
    void SetFormantShift(float shift);

    int GetNumBins() const { return num_bans_; }
    qwqdsp_simd_element::PackFloat<2> GetBinPeak(size_t idx) const {
        auto v = main_peaks_[idx / 4];
        return {v[0][idx & 3], v[1][idx & 3]};
    }
private:
    void UpdateFilters();

    template<class AssignMap>
    void _UpdateFilters();

    template<class AssignMap, class Designer>
    void _UpdateFilters2();

    template<size_t kFilterNumbers, bool kOnlyPole>
    void _ProcessBlock(
        qwqdsp_simd_element::PackFloat<2>* main,
        qwqdsp_simd_element::PackFloat<2>* side,
        size_t num_samples
    );

    float carry_w_mul_{1.0f};
    float gate_peak_{0.0f};
    float gain_{1.0f};
    FilterBankMode filter_bank_mode_;
    float sample_rate_{48000.0f};
    float freq_begin_{40.0f};
    float freq_end_{12000.0f};
    int num_bans_{16};
    size_t num_filters_{4};
    float attack_{1.0f};
    float release_{150.0f};
    float scale_{1.0f};
    float carry_scale_{1.0f};
    float attack_ms_{};
    float release_ms_{};
    eChannelVocoderMap map_{};
    std::array<CascadeBPSVF, kMaxOrder> main_filters_;
    std::array<CascadeBPSVF, kMaxOrder> side_filters_;
    std::array<qwqdsp_simd_element::PackFloat<4>[2], kMaxOrder> main_peaks_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 256> output_{};
};

}
