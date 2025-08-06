#pragma once
#include "param_ids.hpp"
#include <span>
#include <array>
#include <cmath>
#include <vector>

namespace dsp {

struct BandSVF {
    float m1{};
    float ic2eq{};
    float ic1eq{};
    float a1{};
    float a2{};
    float a3{};

    float Tick(float v0) {
        float v3 = v0 - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2 * v1 - ic1eq;
        ic2eq = 2 * v2 - ic2eq;
        float out = v1;
        return out;
    }

    void MakeBandpass(float omega, float Q) {
        float g = std::tan(omega / 2);
        float k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void Reset() {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }
};

template<class T>
class CascadeBPSVF {
public:
    static constexpr int kNumCascade = 4;

    static float DbToGain(float db) {
        return std::pow(10.0f, db / 20.0f);
    }

    // From https://github.com/ZL-Audio/ZLEqualizer
    void MakeBandpass(float omega, float Q) {
        const auto halfbw = std::asinh(0.5f / Q) / std::log(2.0f);
        const auto w = omega / std::pow(2.0f, halfbw);
        const auto g = DbToGain(-6 / static_cast<float>(kNumCascade * 2));
        const auto _q = std::sqrt(1 - g * g) * w * omega / g / (omega * omega - w * w);
        gain_ = 1.0f;
        for (auto& f : svf_) {
            f.MakeBandpass(omega, _q);
            // this will keep the spectrum volume
            // the time-domain volume will change a bit
            gain_ *= 1.0f / _q;
        }
    }

    float Tick(float x) {
        for (auto& f : svf_) {
            x = f.Tick(x);
        }
        return x * gain_;
    }

    void Reset() {
        for (auto& f : svf_) {
            f.Reset();
        }
    }

    float gain_{};
private:
    T svf_[kNumCascade];
};

class ChannelVocoder {
public:
    static constexpr int kMaxOrder = 100;
    static constexpr int kMinOrder = 4;

    void Init(float sample_rate);
    void ProcessBlock(std::span<float> block, std::span<float> side);

    void SetNumBands(int bands);
    void SetFreqBegin(float begin);
    void SetFreqEnd(float end);
    void SetAttack(float attack);
    void SetRelease(float release);
    void SetModulatorScale(float scale);
    void SetCarryScale(float scale);
    void SetMap(eChannelVocoderMap map);

    int GetNumBins() const { return num_bans_; }
    float GetBinPeak(int idx) const { return main_peaks_[idx]; }

private:
    void UpdateFilters();
    template<class AssignMap>
    void _UpdateFilters();

    float sample_rate_{};
    float freq_begin_{ 40.0f };
    float freq_end_{ 12000.0f };
    int num_bans_{ 4 };
    float attack_{};
    float release_{};
    float scale_{1.0f};
    float carry_scale_{1.0f};
    float gain_{};
    eChannelVocoderMap map_{};
    std::array<CascadeBPSVF<BandSVF>, kMaxOrder> main_filters_;
    std::array<CascadeBPSVF<BandSVF>, kMaxOrder> side_filters_;
    std::array<float, kMaxOrder> main_peaks_{};
    std::vector<float> output_;
};

}