#pragma once
#include "param_ids.hpp"
#include <array>
#include <cmath>
#include <vector>
#include <numbers>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/filter/iir_design_extra.hpp>
#include <qwqdsp/filter/iir_design.hpp>

namespace green_vocoder::dsp {

struct BandSVF {
    qwqdsp_simd_element::PackFloat<4> ic2eq_l{};
    qwqdsp_simd_element::PackFloat<4> ic2eq_r{};
    qwqdsp_simd_element::PackFloat<4> ic1eq_l{};
    qwqdsp_simd_element::PackFloat<4> ic1eq_r{};
    qwqdsp_simd_element::PackFloat<4> a1{};
    qwqdsp_simd_element::PackFloat<4> a2{};
    qwqdsp_simd_element::PackFloat<4> a3{};
    qwqdsp_simd_element::PackFloat<4> r2{};

    template<bool kNormal>
    void Tick(
        qwqdsp_simd_element::PackFloat<4>& v0_l,
        qwqdsp_simd_element::PackFloat<4>& v0_r
    ) {
        auto v3_l = v0_l * r2 - ic2eq_l;
        auto v3_r = v0_r * r2 - ic2eq_r;
        auto v1_l = a1 * ic1eq_l + a2 * v3_l;
        auto v1_r = a1 * ic1eq_r + a2 * v3_r;
        auto v2_l = ic2eq_l + a2 * ic1eq_l + a3 * v3_l;
        auto v2_r = ic2eq_r + a2 * ic1eq_r + a3 * v3_r;
        ic1eq_l = 2 * v1_l - ic1eq_l;
        ic1eq_r = 2 * v1_r - ic1eq_r;
        ic2eq_l = 2 * v2_l - ic2eq_l;
        ic2eq_r = 2 * v2_r - ic2eq_r;
        v0_l = v1_l;
        v0_r = v1_r;
    }

    template<bool kOmegaIsDigital>
    void MakeBandpass(qwqdsp_simd_element::PackFloatCRef<4> omega, qwqdsp_simd_element::PackFloatCRef<4> Q) {
        if constexpr (kOmegaIsDigital) {
            auto g = qwqdsp_simd_element::PackOps::Tan(omega / 2);
            auto k = 1.0f / Q;
            r2 = k;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
        else {
            auto g = omega;
            auto k = 1.0f / Q;
            r2 = k;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
    }

    void Reset() {
        ic1eq_l.Broadcast(0);
        ic2eq_l.Broadcast(0);
        ic1eq_r.Broadcast(0);
        ic2eq_r.Broadcast(0);
    }
};

class CascadeBPSVF {
public:
    template<size_t kOrder, bool kNorm>
    void Tick(qwqdsp_simd_element::PackFloat<4>& l, qwqdsp_simd_element::PackFloat<4>& r) {
        for (size_t i = 0; i < kOrder; ++i) {
            svf_[i].Tick<kNorm>(l, r);
        }
    }

    void Reset() {
        for (auto& f : svf_) {
            f.Reset();
        }
    }

    std::array<BandSVF, 4> svf_;
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
        Chebyshev24
    };

    void Init(float sample_rate, size_t block_size);
    void ProcessBlock(
        qwqdsp_simd_element::PackFloat<2>* main,
        qwqdsp_simd_element::PackFloat<2>* side,
        size_t num_samples
    );

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

    template<size_t kFilterNumbers>
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
    eChannelVocoderMap map_{};
    std::array<CascadeBPSVF, kMaxOrder> main_filters_;
    std::array<CascadeBPSVF, kMaxOrder> side_filters_;
    std::array<qwqdsp_simd_element::PackFloat<4>[2], kMaxOrder> main_peaks_{};
    std::vector<qwqdsp_simd_element::PackFloat<2>> output_;
};

}
