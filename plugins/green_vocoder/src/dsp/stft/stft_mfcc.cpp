#include "stft_mfcc.hpp"

#include <algorithm>
#include <cmath>

#include <qwqdsp/convert.hpp>
#include "qwqdsp/interpolation.hpp"

namespace green_vocoder::dsp {

void STFTMFCC::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTMFCC::SetParam(const Params& p, STFT& self) {
    int const fft_size = self.fft_size_;
    size_t num_bins = static_cast<size_t>(fft_size) / 2 + 1;
    fill_gains_.resize(num_bins + global::kExtraGainSize);

    // mel 频带
    num_mfcc_ = std::clamp(p.num_mfcc, global::kMinNumMfcc, global::kMaxNumMfcc);
    float begin_mel = qwqdsp::convert::Freq2Mel(0);
    float end_mel = qwqdsp::convert::Freq2Mel(self.sample_rate_ / 2);
    float interval_mel = (end_mel - begin_mel) / static_cast<float>(num_mfcc_);
    for (int i = 0; i < num_mfcc_; ++i) {
        float mel = begin_mel + static_cast<float>(i) * interval_mel;
        float freq = qwqdsp::convert::Mel2Freq(mel);
        int bin = static_cast<int>(std::floor(freq / static_cast<float>(self.sample_rate_) * static_cast<float>(fft_size)));
        bin = std::min(bin, fft_size / 2);
        mfcc_indexs_[static_cast<size_t>(i)] = static_cast<size_t>(bin);
    }
    mfcc_indexs_[0] = 0;
    mfcc_indexs_[static_cast<size_t>(num_mfcc_)] = static_cast<size_t>(fft_size) / 2;
}

void STFTMFCC::operator()(STFT& self,
                          std::span<const float> real_in, std::span<const float> imag_in,
                          std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.mfcc_gains_ : self.mfcc_gains2_;
    int const fft_size = self.fft_size_;
    for (size_t mcff_idx = 0; mcff_idx < static_cast<size_t>(num_mfcc_); ++mcff_idx) {
        size_t begin = mfcc_indexs_[mcff_idx];
        size_t end = mfcc_indexs_[mcff_idx + 1];

        // 频带 RMS 作为声码器带增益
        float sum = 0.0f;
        for (size_t i = begin; i < end; ++i) {
            sum += real_in[i] * real_in[i] + imag_in[i] * imag_in[i];
        }
        sum /= static_cast<float>(end - begin + 1);
        sum = std::sqrt(sum);

        float gain = sum * self.window_gain_;
        if (gain > gains[mcff_idx]) {
            gains[mcff_idx] = self.attack_factor_ * gains[mcff_idx] + (1.0f - self.attack_factor_) * gain;
        }
        else {
            gains[mcff_idx] = self.decay_ * gains[mcff_idx] + (1.0f - self.decay_) * gain;
        }

        for (size_t i = begin; i < end; ++i) {
            fill_gains_[i] = gains[mcff_idx];
        }
    }

    size_t num_bins = static_cast<size_t>(fft_size) / 2 + 1;
    fill_gains_[num_bins] = fill_gains_[0];
    // 共振峰搬移
    for (size_t i = 0; i < num_bins; ++i) {
        float idx = static_cast<float>(i) * self.formant_mul_;
        float frac = idx - std::floor(idx);
        size_t iidx = static_cast<size_t>(idx);

        float g = 0;
        if (iidx < num_bins) {
            g = qwqdsp::Interpolation::Linear(fill_gains_[iidx], fill_gains_[iidx + 1], frac);
        }

        real_out[i] *= g;
        imag_out[i] *= g;
    }
}

} // namespace green_vocoder::dsp
