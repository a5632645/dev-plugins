#include "stft_standard.hpp"

#include <algorithm>
#include <cmath>

#include "qwqdsp/interpolation.hpp"

namespace green_vocoder::dsp {

void STFTStandard::operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                              std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;

    int const num_bins = self.fft_size_ / 2 + 1;
    for (int i = 0; i < num_bins; ++i) {
        float power = real_in[static_cast<size_t>(i)] * real_in[static_cast<size_t>(i)]
                    + imag_in[static_cast<size_t>(i)] * imag_in[static_cast<size_t>(i)];
        float gain = std::sqrt(power) * self.hann_sinc_window_gain_ * global::kStftModMakeup;
        gain = self.Blend(gain);

        if (gain > gains[static_cast<size_t>(i)]) {
            gains[static_cast<size_t>(i)] =
                self.attack_factor_ * gains[static_cast<size_t>(i)] + (1.0f - self.attack_factor_) * gain;
        }
        else {
            gains[static_cast<size_t>(i)] =
                self.decay_factor_ * gains[static_cast<size_t>(i)] + (1.0f - self.decay_factor_) * gain;
        }
    }
    gains[static_cast<size_t>(num_bins)] = gains[0];

    // 共振峰搬移
    for (int i = 0; i < num_bins; ++i) {
        // idx 超出奈奎斯特时 clamp 到末 bin，避免向下搬移时顶部频谱被置零
        float idx = std::min(static_cast<float>(i) * self.formant_mul_,
                             static_cast<float>(num_bins - 1));
        float const frac = idx - std::floor(idx);
        int const iidx = static_cast<int>(idx);

        float const g = qwqdsp::Interpolation::Linear(gains[static_cast<size_t>(iidx)],
                                                      gains[static_cast<size_t>(iidx) + 1], frac);

        real_out[static_cast<size_t>(i)] *= g;
        imag_out[static_cast<size_t>(i)] *= g;
    }
}

} // namespace green_vocoder::dsp
