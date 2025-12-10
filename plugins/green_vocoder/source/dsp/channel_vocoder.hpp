#pragma once
#include "param_ids.hpp"
#include <array>
#include <cmath>
#include <vector>
#include <numbers>
#include <qwqdsp/simd_element/simd_pack.hpp>

namespace green_vocoder::dsp {

struct BandSVF {
    qwqdsp_simd_element::PackFloat<4> ic2eq_l{};
    qwqdsp_simd_element::PackFloat<4> ic2eq_r{};
    qwqdsp_simd_element::PackFloat<4> ic1eq_l{};
    qwqdsp_simd_element::PackFloat<4> ic1eq_r{};
    qwqdsp_simd_element::PackFloat<4> a1{};
    qwqdsp_simd_element::PackFloat<4> a2{};
    qwqdsp_simd_element::PackFloat<4> a3{};

    void Tick(
        qwqdsp_simd_element::PackFloat<4>& v0_l,
        qwqdsp_simd_element::PackFloat<4>& v0_r
    ) {
        auto v3_l = v0_l - ic2eq_l;
        auto v3_r = v0_r - ic2eq_r;
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

    void MakeBandpass(qwqdsp_simd_element::PackFloat<4> omega, qwqdsp_simd_element::PackFloat<4> Q) {
        auto g = qwqdsp_simd_element::PackOps::Tan(omega / 2);
        auto k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
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
    static constexpr int kNumCascade = 4; // 48dB/oct

    static float DbToGain(float db) {
        return std::pow(10.0f, db / 20.0f);
    }

    // From https://github.com/ZL-Audio/ZLEqualizer
    void MakeBandpass(qwqdsp_simd_element::PackFloatCRef<4> omega, qwqdsp_simd_element::PackFloatCRef<4> bw) {
        const auto Q = omega / bw;
        const auto halfbw = qwqdsp_simd_element::PackOps::Asinh(0.5f / Q) / std::numbers::ln2_v<float>;
        const auto w = omega / qwqdsp_simd_element::PackOps::Exp2(halfbw);
        const auto g = DbToGain(-6 / static_cast<float>(kNumCascade * 2));
        const auto _q = std::sqrt(1 - g * g) * w * omega / g / (omega * omega - w * w);
        gain_.Broadcast(1);
        for (auto& f : svf_) {
            f.MakeBandpass(omega, _q);
            // this will keep the spectrum volume
            // the time-domain volume will change a bit
            gain_ *= 1.0f / _q;
        }
    }

    void Tick(qwqdsp_simd_element::PackFloat<4>& l, qwqdsp_simd_element::PackFloat<4>& r) {
        for (auto& f : svf_) {
            f.Tick(l, r);
        }
        l *= gain_;
        r *= gain_;
    }

    void Reset() {
        for (auto& f : svf_) {
            f.Reset();
        }
    }

    qwqdsp_simd_element::PackFloat<4> gain_{};
private:
    BandSVF svf_[kNumCascade];
};

class ChannelVocoder {
public:
    static constexpr int kMaxOrder = 100;
    static constexpr int kMinOrder = 4;

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

    int GetNumBins() const { return num_bans_; }
    qwqdsp_simd_element::PackFloat<2> GetBinPeak(size_t idx) const {
        auto v = main_peaks_[idx / 4];
        return {v[0][idx & 3], v[1][idx & 3]};
    }
private:
    void UpdateFilters();
    template<class AssignMap>
    void _UpdateFilters();

    float sample_rate_{};
    float freq_begin_{ 40.0f };
    float freq_end_{ 12000.0f };
    int num_bans_{ 4 };
    size_t num_filters_{1};
    float attack_{};
    float release_{};
    float scale_{1.0f};
    float carry_scale_{1.0f};
    float gain_{};
    eChannelVocoderMap map_{};
    std::array<CascadeBPSVF, kMaxOrder> main_filters_;
    std::array<CascadeBPSVF, kMaxOrder> side_filters_;
    std::array<qwqdsp_simd_element::PackFloat<4>[2], kMaxOrder> main_peaks_{};
    std::vector<qwqdsp_simd_element::PackFloat<2>> output_;
};

}
