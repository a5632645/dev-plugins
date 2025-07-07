#pragma once
#include <span>
#include <array>
#include "svf.hpp"

/* TODO
 *   volume explode when high bands and low bandwidth, should has a normalization gain
*/

namespace dsp {

class ChannelVocoder {
public:
    static constexpr int kMaxOrder = 40;

    class CascadeBPSVF {
    public:
        static constexpr int kNumCascade = 2;
        void MakeBandpass(float omega, float q) {
            /* a simple cascade method
             * https://www.analog.com/media/en/technical-documentation/application-notes/an27af.pdf
            */
            q = q * std::sqrt(std::exp2(1.0f / kNumCascade) - 1.0f);
            for (auto& f : svf_) {
                f.MakeBandpass(omega, q);
            }
        }

        float Tick(float x) {
            for (auto& f : svf_) {
                x = f.Tick(x);
            }
            return x;
        }
    private:
        SVF svf_[kNumCascade];
    };

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
    std::array<CascadeBPSVF, kMaxOrder> main_filters_;
    std::array<CascadeBPSVF, kMaxOrder> side_filters_;
    std::array<float, kMaxOrder> main_peaks_{};
};

}