#include "stft_standard.hpp"

#include <cmath>

#include "qwqdsp/interpolation.hpp"

namespace green_vocoder::dsp {

void STFTStandard::operator()(STFT& self,
                              std::span<const float> real_in, std::span<const float> imag_in,
                              std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;
    int const num_bins = self.fft_size_ / 2 + 1;
    for (int i = 0; i < num_bins; ++i) {
        float power = std::abs(real_in[static_cast<size_t>(i)] * real_in[static_cast<size_t>(i)]
                               + imag_in[static_cast<size_t>(i)] * imag_in[static_cast<size_t>(i)]);
        float gain = std::sqrt(power) * self.window_gain_;
        gain = self.Blend(gain);

        if (gain > gains[static_cast<size_t>(i)]) {
            gains[static_cast<size_t>(i)] = self.attack_factor_ * gains[static_cast<size_t>(i)]
                                          + (1.0f - self.attack_factor_) * gain;
        }
        else {
            gains[static_cast<size_t>(i)] = self.decay_ * gains[static_cast<size_t>(i)]
                                          + (1.0f - self.decay_) * gain;
        }
    }
    gains[static_cast<size_t>(num_bins)] = gains[0];
    // 共振峰搬移
    for (int i = 0; i < num_bins; ++i) {
        float idx = static_cast<float>(i) * self.formant_mul_;
        float frac = idx - std::floor(idx);
        int iidx = static_cast<int>(idx);

        float g = 0;
        if (iidx < num_bins) {
            g = qwqdsp::Interpolation::Linear(gains[static_cast<size_t>(iidx)],
                                              gains[static_cast<size_t>(iidx) + 1], frac);
        }

        real_out[static_cast<size_t>(i)] *= g;
        imag_out[static_cast<size_t>(i)] *= g;
    }
}

} // namespace green_vocoder::dsp
