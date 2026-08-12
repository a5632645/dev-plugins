#include "stft_smooth.hpp"

#include <algorithm>
#include <cmath>

#include "qwqdsp/interpolation.hpp"

namespace green_vocoder::dsp {

void STFTSmooth::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTSmooth::SetParam(const Params& p, STFT& self) {
    int const fft_size = self.fft_size_;
    bool const size_changed = fft_size != fft_size_;
    // 仅当 fft_size 改变时才重建功率谱缓冲与平滑器（分配开销大）
    if (size_changed) {
        fft_size_ = fft_size;
        int const num_bins = fft_size / 2 + 1;
        power_.resize(static_cast<size_t>(num_bins));
        smoother_.prepare(static_cast<size_t>(fft_size));
    }

    // 平滑类型/量变化，或尺寸变化（prepare 已清零索引表，需随新尺寸重建）时重算平滑器索引
    if (size_changed || p.type != type_ || ParamChanged(p.amount, amount_)) {
        type_ = p.type;
        amount_ = p.amount;
        if (type_ == SmoothType::ERB)
            smoother_.setSmoothERB(self.sample_rate_, amount_);
        else
            smoother_.setSmoothOCT(amount_);
    }
}

void STFTSmooth::operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                            std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;
    int const num_bins = self.fft_size_ / 2 + 1;

    // 调制器功率谱 → OCT/ERB 平滑包络（就地平滑）
    for (int i = 0; i < num_bins; ++i) {
        power_[static_cast<size_t>(i)] = real_in[static_cast<size_t>(i)] * real_in[static_cast<size_t>(i)]
                                       + imag_in[static_cast<size_t>(i)] * imag_in[static_cast<size_t>(i)];
    }
    smoother_.smooth(power_);

    for (int i = 0; i < num_bins; ++i) {
        float gain = std::sqrt(power_[static_cast<size_t>(i)]) * self.hann_window_gain_ * global::kStftModMakeup;
        gain = self.Blend(gain);

        if (gain > gains[static_cast<size_t>(i)]) {
            gains[static_cast<size_t>(i)] =
                self.attack_factor_ * gains[static_cast<size_t>(i)] + (1.0f - self.attack_factor_) * gain;
        }
        else {
            gains[static_cast<size_t>(i)] = self.decay_factor_ * gains[static_cast<size_t>(i)] + (1.0f - self.decay_factor_) * gain;
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
