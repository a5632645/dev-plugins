#pragma once

#include <qwqdsp/oscillator/dsf_correct.hpp>
#include <qwqdsp/convert.hpp>

namespace ewarp {
class Ewarp {
public:
    static constexpr float kBaseFreq = 20000.0f;

    void Reset() noexcept {
        dsf_.Reset();
        reverse_spectrum_gain_ = 1.0f;
    }

    void Process(float* left, float* right, size_t num_samples) noexcept {
        float dry_mix = 1.0f - am2rm;
        float wet_mix = am2rm;
        float normal_mix = 1.0f - reverse_mix_;
        float reverse_mix = reverse_mix_;
        for (size_t i = 0; i < num_samples; ++i) {
            float x0 = left[i];
            float x1 = right[i];
            float reverse_x0 = x0 * reverse_spectrum_gain_;
            float reverse_x1 = x1 * reverse_spectrum_gain_;
            float avg_x0 = normal_mix * x0 + reverse_mix * reverse_x0;
            float avg_x1 = normal_mix * x1 + reverse_mix * reverse_x1;
            float mod = dsf_.Tick() * wet_mix + dry_mix;
            left[i] = avg_x0 * mod;
            right[i] = avg_x1 * mod;
            reverse_spectrum_gain_ = -reverse_spectrum_gain_;
        }
    }

    void Update() noexcept {
        float base_freq = kBaseFreq / warp_bands;
        float w0 = qwqdsp::convert::Freq2W(base_freq, fs);
        float w = w0 * ratio;
        dsf_.SetW0(w0);
        dsf_.SetWSpace(w);
        dsf_.SetAmpFactor(decay);
        int max_n = static_cast<int>((std::numbers::pi_v<float> - w0) / w);
        max_n = std::max<int>(max_n, 1);
        uint32_t uint_max_n = static_cast<uint32_t>(max_n);
        dsf_.SetN(uint_max_n);
        normalize_gain_ = dsf_.NormalizeGain();
    }

    float fs{48000.0f};
    float warp_bands{100.0f};
    float ratio{1.0f};
    float am2rm{0.5f};
    float decay{0.9f};
    float reverse_mix_{0.5f};
private:
    float normalize_gain_{};
    float reverse_spectrum_gain_{1.0f};
    qwqdsp_oscillator::DSFCorrect<> dsf_;
};
}
