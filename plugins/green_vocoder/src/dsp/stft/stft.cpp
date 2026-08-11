#include "stft.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>

#include <qwqdsp/convert.hpp>

namespace green_vocoder::dsp {

void STFT::Init(float fs) {
    sample_rate_ = fs;
    SetParam(Params{});
}

void STFT::Reset() {
    std::ranges::fill(temp_main_, 0.0f);
    std::ranges::fill(temp_side_, 0.0f);
    std::ranges::fill(real_main_, 0.0f);
    std::ranges::fill(real_side_, 0.0f);
    std::ranges::fill(imag_main_, 0.0f);
    std::ranges::fill(imag_side_, 0.0f);
    std::ranges::fill(gains_, 0.0f);
    std::ranges::fill(gains2_, 0.0f);
    std::ranges::fill(mfcc_gains_, 0.0f);
    std::ranges::fill(mfcc_gains2_, 0.0f);
}

void STFT::SetParam(const Params& p) {
    // 仅当 fft_size 改变时才重建 FFT/窗口/缓冲（分配与重算开销大）
    bool const size_changed = p.fft_size != fft_size_;
    if (size_changed) {
        fft_size_ = p.fft_size;
        fft_.Init(static_cast<size_t>(p.fft_size));

        // hann 窗（分析与合成共用）
        hann_window_.resize(static_cast<size_t>(p.fft_size));
        for (int i = 0; i < p.fft_size; ++i) {
            hann_window_[static_cast<size_t>(i)] =
                0.5f
                - 0.5f
                      * std::cos(2.0f * std::numbers::pi_v<float>
                                 * static_cast<float>(i) / static_cast<float>(p.fft_size));
        }

        // 缓冲
        int const num_bins = p.fft_size / 2 + 1;
        temp_main_.resize(static_cast<size_t>(p.fft_size) * 2);
        temp_side_.resize(static_cast<size_t>(p.fft_size) * 2);
        real_main_.resize(static_cast<size_t>(num_bins));
        real_side_.resize(static_cast<size_t>(num_bins));
        imag_main_.resize(static_cast<size_t>(num_bins));
        imag_side_.resize(static_cast<size_t>(num_bins));
        output_frame_.resize(static_cast<size_t>(p.fft_size));
        gains_.resize(static_cast<size_t>(num_bins) + global::kExtraGainSize);
        gains2_.resize(static_cast<size_t>(num_bins) + global::kExtraGainSize);
        window_.resize(static_cast<size_t>(p.fft_size));
    }

    // sinc*hann（bandwidth）窗与重建增益（仅 bandwidth 或 fft_size 变化时重算）
    if (size_changed || ParamChanged(p.bandwidth, bandwidth_)) {
        bandwidth_ = p.bandwidth;
        float const f0 = p.bandwidth * static_cast<float>(p.fft_size) / 1024.0f;
        for (int i = 0; i < p.fft_size; ++i) {
            float const x = (2.0f * std::numbers::pi_v<float>
                             * f0 * (static_cast<float>(i) - static_cast<float>(p.fft_size) / 2.0f))
                          / static_cast<float>(p.fft_size);
            float const sinc = std::abs(x) < 1e-6f ? 1.0f : std::sin(x) / x;
            window_[static_cast<size_t>(i)] = sinc * hann_window_[static_cast<size_t>(i)];
        }
        window_gain_ = 2.0f / std::accumulate(window_.begin(), window_.end(), 0.0f);
    }

    // attack / release（依赖 hop_size_）
    attack_factor_ = qwqdsp::convert::Ms2DecayDb(p.attack, sample_rate_, -60.0f);
    decay_ = qwqdsp::convert::Ms2DecayDb(p.release + p.attack, sample_rate_ / (static_cast<float>(fft_size_) / 4.0f),
                                         -60.0f);

    blend_ = p.blend;
    formant_mul_ = std::exp2(-p.formant_shift / 12.0f);
}

float STFT::Blend(float x) noexcept {
    x = 2.0f * x - 1.0f;
    x = (blend_ + x) / (1.0f + blend_ * x);
    x = 0.5f * x + 0.5f;
    return x;
}

} // namespace green_vocoder::dsp
