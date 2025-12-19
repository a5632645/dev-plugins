#pragma once
#include "param_ids.hpp"
#include <array>
#include <vector>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/filter/iir_design_extra.hpp>
#include <qwqdsp/filter/iir_design.hpp>

namespace green_vocoder::dsp {
class BandSVF {
public:
    void Tick(
        qwqdsp_simd_element::PackFloat<4>& v0_l,
        qwqdsp_simd_element::PackFloat<4>& v0_r
    ) noexcept {
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

    void MakeBandpass(
        qwqdsp_simd_element::PackFloatCRef<4> analog_w,
        qwqdsp_simd_element::PackFloatCRef<4> Q
    ) noexcept {
        g_ = analog_w;
        r2_ = 1 / Q;
        d_ = 1 / (1 + r2_ * g_ + g_ * g_);
    }

    void SetAnalogPole(
        qwqdsp_simd_element::PackFloatCRef<4> real,
        qwqdsp_simd_element::PackFloatCRef<4> imag
    ) noexcept {
        auto analog_w = qwqdsp_simd_element::PackOps::Sqrt(real * real + imag * imag);
        auto Q = analog_w / (-2.0f * real);
        MakeBandpass(analog_w, qwqdsp_simd_element::PackOps::Abs(Q));
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
};

struct CascadeBPSVF {
    template<size_t kNumFilters>
    void Tick(
        qwqdsp_simd_element::PackFloat<4>& l,
        qwqdsp_simd_element::PackFloat<4>& r
    ) noexcept {
        for (size_t i = 0; i < kNumFilters; ++i) {
            svf_[i].Tick(l, r);
        }
    }

    void Reset() noexcept {
        for (auto& f : svf_) {
            f.Reset();
        }
    }

    std::array<BandSVF, 4> svf_;
};

class StereoBiquad {
public:
    void Tick(
        qwqdsp_simd_element::PackFloat<4>& l,
        qwqdsp_simd_element::PackFloat<4>& r
    ) noexcept {
        auto y_l = l * b0_ + s1_l_;
        auto y_r = r * b0_ + s1_r_;
        s1_l_ = l * b1_ - y_l * a1_ + s2_l_;
        s1_r_ = r * b1_ - y_r * a1_ + s2_r_;
        s2_l_ = l * b2_ - y_l * a2_;
        s2_r_ = r * b2_ - y_r * a2_;
        l = y_l;
        r = y_r;
    }

    void Reset() noexcept {
        s1_l_.Broadcast(0);
        s1_r_.Broadcast(0);
        s2_r_.Broadcast(0);
        s2_l_.Broadcast(0);
    }

    void Set(size_t idx, qwqdsp_filter::BiquadCoeff const& coeffs) {
        b0_[idx] = coeffs.b0;
        b1_[idx] = coeffs.b1;
        b2_[idx] = coeffs.b2;
        a1_[idx] = coeffs.a1;
        a2_[idx] = coeffs.a2;
    }
private:
    qwqdsp_simd_element::PackFloat<4> s1_l_{};
    qwqdsp_simd_element::PackFloat<4> s1_r_{};
    qwqdsp_simd_element::PackFloat<4> s2_l_{};
    qwqdsp_simd_element::PackFloat<4> s2_r_{};
    qwqdsp_simd_element::PackFloat<4> a1_{};
    qwqdsp_simd_element::PackFloat<4> a2_{};
    qwqdsp_simd_element::PackFloat<4> b0_{};
    qwqdsp_simd_element::PackFloat<4> b1_{};
    qwqdsp_simd_element::PackFloat<4> b2_{};
};

// i don't figure out how to add zero to SVF
// so we fallback to TDF2 biquad
struct CascadeBiquad {
    template<size_t kNumFilters>
    void Tick(
        qwqdsp_simd_element::PackFloat<4>& l,
        qwqdsp_simd_element::PackFloat<4>& r
    ) noexcept {
        for (size_t i = 0; i < kNumFilters; ++i) {
            biquads_[i].Tick(l, r);
        }
    }

    void Reset() noexcept {
        for (auto& f : biquads_) {
            f.Reset();
        }
    }

    template<size_t kNumFilters>
    void Set(size_t idx, std::array<qwqdsp_filter::BiquadCoeff, kNumFilters> const& coeffs) {
        for (size_t i = 0; i < kNumFilters; ++i) {
            biquads_[i].Set(idx, coeffs[i]);
        }
    }

    std::array<StereoBiquad, 6> biquads_;
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

    template<size_t kFilterNumbers, bool kHaveZero>
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
    std::array<CascadeBiquad, kMaxOrder> main_filters_with_zero_;
    std::array<CascadeBiquad, kMaxOrder> side_filters_with_zero_;
    std::array<qwqdsp_simd_element::PackFloat<4>[2], kMaxOrder> main_peaks_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 256> output_{};
};

}
