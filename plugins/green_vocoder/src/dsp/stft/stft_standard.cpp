#include "stft_standard.hpp"

#include <cmath>

#include "qwqdsp/interpolation.hpp"

namespace green_vocoder::dsp {

void STFTStandard::operator()(STFT& self,
                              std::span<const float> real_in, std::span<const float> imag_in,
                              std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;
    size_t num_bins = static_cast<size_t>(self.fft_size_) / 2 + 1;
    for (size_t i = 0; i < num_bins; ++i) {
        float power = std::abs(real_in[i] * real_in[i] + imag_in[i] * imag_in[i]);
        float gain = std::sqrt(power) * self.window_gain_;
        gain = self.Blend(gain);

        if (gain > gains[i]) {
            gains[i] = self.attack_factor_ * gains[i] + (1.0f - self.attack_factor_) * gain;
        }
        else {
            gains[i] = self.decay_ * gains[i] + (1.0f - self.decay_) * gain;
        }
    }
    gains[num_bins] = gains[0];
    // 共振峰搬移
    for (size_t i = 0; i < num_bins; ++i) {
        float idx = static_cast<float>(i) * self.formant_mul_;
        float frac = idx - std::floor(idx);
        size_t iidx = static_cast<size_t>(idx);

        float g = 0;
        if (iidx < num_bins) {
            g = qwqdsp::Interpolation::Linear(gains[iidx], gains[iidx + 1], frac);
        }

        real_out[i] *= g;
        imag_out[i] *= g;
    }
}

} // namespace green_vocoder::dsp
