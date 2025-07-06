/*
 * TODO: change filter Q to bandwidth(like overlap?)
*/

#pragma once
#include <span>
#include <cmath>
#include <array>

namespace dsp {

class ChannelVocoder {
public:
    static constexpr int kMaxOrder = 40;

    class BandpassFilter {
    public:
        void Set(float omega, float bw) {
            float q = omega / bw;
            auto k = std::tan(omega / 2);
            auto down = k * k * q + k + q;
            b0_ = k * q / down;
            b2_ = -b0_;
            a1_ = 2 * q * (k * k - 1) / down;
            a2_ = (k * k * q - k + q) / down;
        }

        float ProcessSingle(float x) {
            auto latch0 = x - a1_ * latch1_ - a2_ * latch2_;
            auto y = b0_ * latch0 + b2_ * latch2_;
            latch2_ = latch1_;
            latch1_ = latch0;
            return y;
        }

        void CopyCoeffients(const BandpassFilter& other) {
            b0_ = other.b0_;
            b2_ = other.b2_;
            a1_ = other.a1_;
            a2_ = other.a2_;
        }

        void ClearLatch() {
            latch1_ = 0.0f;
            latch2_ = 0.0f;
        }
    private:
        float b0_{};
        float b2_{};
        float a1_{};
        float a2_{};
        float latch1_{};
        float latch2_{};
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
    std::array<BandpassFilter, kMaxOrder> main_filters_;
    std::array<BandpassFilter, kMaxOrder> side_filters_;
    std::array<float, kMaxOrder> main_peaks_{};
};

}