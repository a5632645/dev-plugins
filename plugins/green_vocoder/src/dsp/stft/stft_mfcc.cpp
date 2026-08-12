#include "stft_mfcc.hpp"

#include <algorithm>
#include <cmath>

#include "qwqdsp/interpolation.hpp"
#include <qwqdsp/convert.hpp>

namespace green_vocoder::dsp {

void STFTMFCC::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTMFCC::SetParam(const Params& p, STFT& self) {
    int const fft_size = self.fft_size_;
    // 仅当 fft_size 改变时才重建频带填充缓冲
    if (fft_size != fft_size_) {
        fft_size_ = fft_size;
        int const num_bins = fft_size / 2 + 1;
        fill_gains_.resize(static_cast<size_t>(num_bins) + global::kExtraGainSize);
    }

    // mel 频带（num_mfcc 变化时始终重算）
    num_mfcc_ = std::clamp(p.num_mfcc, global::kMinNumMfcc, global::kMaxNumMfcc);
    float begin_mel = qwqdsp::convert::Freq2Mel(0);
    float end_mel = qwqdsp::convert::Freq2Mel(self.sample_rate_ / 2);
    float interval_mel = (end_mel - begin_mel) / static_cast<float>(num_mfcc_);
    for (int i = 0; i < num_mfcc_; ++i) {
        float mel = begin_mel + static_cast<float>(i) * interval_mel;
        float freq = qwqdsp::convert::Mel2Freq(mel);
        int bin =
            static_cast<int>(std::floor(freq / static_cast<float>(self.sample_rate_) * static_cast<float>(fft_size)));
        bin = std::min(bin, fft_size / 2);
        mfcc_indexs_[static_cast<size_t>(i)] = bin;
    }
    mfcc_indexs_[0] = 0;
    mfcc_indexs_[static_cast<size_t>(num_mfcc_)] = fft_size / 2;
}

void STFTMFCC::operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                          std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.mfcc_gains_left_ : self.mfcc_gains_right_;
    int const fft_size = self.fft_size_;
    for (int mcff_idx = 0; mcff_idx < num_mfcc_; ++mcff_idx) {
        int const begin = mfcc_indexs_[static_cast<size_t>(mcff_idx)];
        int const end = mfcc_indexs_[static_cast<size_t>(mcff_idx) + 1];

        // 频带 RMS 作为声码器带增益（同 Smooth 系：乘 mod 增益补偿）
        float sum = 0.0f;
        for (int i = begin; i < end; ++i) {
            sum += real_in[static_cast<size_t>(i)] * real_in[static_cast<size_t>(i)]
                 + imag_in[static_cast<size_t>(i)] * imag_in[static_cast<size_t>(i)];
        }
        sum /= static_cast<float>(end - begin + 1);
        sum = std::sqrt(sum);

        float gain = sum * self.hann_window_gain_ * global::kStftModMakeup;
        gain = self.Blend(gain);
        if (gain > gains[static_cast<size_t>(mcff_idx)]) {
            gains[static_cast<size_t>(mcff_idx)] =
                self.attack_factor_ * gains[static_cast<size_t>(mcff_idx)] + (1.0f - self.attack_factor_) * gain;
        }
        else {
            gains[static_cast<size_t>(mcff_idx)] =
                self.decay_factor_ * gains[static_cast<size_t>(mcff_idx)] + (1.0f - self.decay_factor_) * gain;
        }

        for (int i = begin; i < end; ++i) {
            fill_gains_[static_cast<size_t>(i)] = gains[static_cast<size_t>(mcff_idx)];
        }
    }

    int const num_bins = fft_size / 2 + 1;
    fill_gains_[static_cast<size_t>(num_bins)] = fill_gains_[0];
    // 共振峰搬移
    for (int i = 0; i < num_bins; ++i) {
        // idx 超出奈奎斯特时 clamp 到末 bin，避免向下搬移时顶部频谱被置零
        float idx = std::min(static_cast<float>(i) * self.formant_mul_,
                             static_cast<float>(num_bins - 1));
        float const frac = idx - std::floor(idx);
        int const iidx = static_cast<int>(idx);

        float const g = qwqdsp::Interpolation::Linear(fill_gains_[static_cast<size_t>(iidx)],
                                                      fill_gains_[static_cast<size_t>(iidx) + 1], frac);

        real_out[static_cast<size_t>(i)] *= g;
        imag_out[static_cast<size_t>(i)] *= g;
    }
}

} // namespace green_vocoder::dsp
