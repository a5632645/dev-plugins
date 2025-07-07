#pragma once
#include <span>
#include <array>
#include "svf.hpp"

namespace dsp {

class ChannelVocoder {
public:
    static constexpr int kMaxOrder = 40;

    void Init(float sample_rate);
    void ProcessBlock(std::span<float> block, std::span<float> side);

    void SetNumBands(int bands);
    void SetFreqBegin(float begin);
    void SetFreqEnd(float end);
    void SetAttack(float attack);
    void SetRelease(float release);
    void SetModulatorScale(float scale);
    void SetCarryScale(float scale);

    int GetNumBins() const { return num_bans_; }
    float GetBinPeak(int idx) const { return main_peaks_[idx]; }

private:
    void UpdateFilters();

    float sample_rate_{};
    float freq_begin_{ 40.0f };
    float freq_end_{ 12000.0f };
    int num_bans_{ 4 };
    float attack_{};
    float release_{};
    float scale_{1.0f};
    float carry_scale_{1.0f};
    std::array<SVF, kMaxOrder> main_filters_;
    std::array<SVF, kMaxOrder> side_filters_;
    std::array<float, kMaxOrder> main_peaks_{};
};

}