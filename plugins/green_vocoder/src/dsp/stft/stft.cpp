#include "stft.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <qwqdsp/convert.hpp>
#include <qwqdsp/window/hann.hpp>
#include <qwqdsp/window/helper.hpp>

#include "stft_wiener.hpp"

namespace green_vocoder::dsp {

void STFT::Init(float fs) {
    sample_rate_ = fs;
    SetParam(Params{});
}

void STFT::Reset() {
    std::ranges::fill(temp_main_, 0.0f);
    std::ranges::fill(temp_side_, 0.0f);
    std::ranges::fill(temp_carry_, 0.0f);
    std::ranges::fill(real_main_, 0.0f);
    std::ranges::fill(real_side_, 0.0f);
    std::ranges::fill(imag_main_, 0.0f);
    std::ranges::fill(imag_side_, 0.0f);
    std::ranges::fill(real_carry_, 0.0f);
    std::ranges::fill(imag_carry_, 0.0f);
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

        // hann 窗（分析与合成共用，不归一化）
        hann_window_.resize(static_cast<size_t>(p.fft_size));
        qwqdsp_window::Hann::Window(hann_window_, true);

        // mod / carry 分析窗重建增益（2 / sum(hann) = 4 / fft_size）
        hann_window_gain_ = qwqdsp_window::Helper::NormalizeGain(hann_window_);

        // 缓冲
        int const num_bins = p.fft_size / 2 + 1;
        temp_main_.resize(static_cast<size_t>(p.fft_size) * 2);
        temp_side_.resize(static_cast<size_t>(p.fft_size) * 2);
        temp_carry_.resize(static_cast<size_t>(p.fft_size) * 2);
        real_main_.resize(static_cast<size_t>(num_bins));
        real_side_.resize(static_cast<size_t>(num_bins));
        imag_main_.resize(static_cast<size_t>(num_bins));
        imag_side_.resize(static_cast<size_t>(num_bins));
        real_carry_.resize(static_cast<size_t>(num_bins));
        imag_carry_.resize(static_cast<size_t>(num_bins));
        output_frame_.resize(static_cast<size_t>(p.fft_size));
        gains_.resize(static_cast<size_t>(num_bins) + global::kExtraGainSize);
        gains2_.resize(static_cast<size_t>(num_bins) + global::kExtraGainSize);
        hann_sinc_window_.resize(static_cast<size_t>(p.fft_size));
    }

    // sinc*hann（bandwidth）窗（基于普通 hann，不归一化，仅 Standard 分析用）
    if (size_changed || ParamChanged(p.bandwidth, bandwidth_)) {
        bandwidth_ = p.bandwidth;
        float const f0 = p.bandwidth * static_cast<float>(p.fft_size) / 1024.0f;
        for (int i = 0; i < p.fft_size; ++i) {
            float const x = (2.0f * std::numbers::pi_v<float>
                             * f0 * (static_cast<float>(i) - static_cast<float>(p.fft_size) / 2.0f))
                          / static_cast<float>(p.fft_size);
            float const sinc = std::abs(x) < 1e-6f ? 1.0f : std::sin(x) / x;
            hann_sinc_window_[static_cast<size_t>(i)] = sinc * hann_window_[static_cast<size_t>(i)];
        }

        // Standard 的 mod 分析窗重建增益（2 / sum(sinc*hann)）
        hann_sinc_window_gain_ = qwqdsp_window::Helper::NormalizeGain(hann_sinc_window_);
    }

    // attack / release（依赖 hop_size_）
    attack_factor_ = qwqdsp::convert::Ms2DecayDb(p.attack, sample_rate_, -60.0f);
    decay_factor_ = qwqdsp::convert::Ms2DecayDb(p.release + p.attack,
                                                sample_rate_ / (static_cast<float>(fft_size_) / 4.0f), -60.0f);

    blend_ = p.blend;
    formant_mul_ = std::exp2(-p.formant_shift / 12.0f);
}

float STFT::Blend(float x) noexcept {
    x = 2.0f * x - 1.0f;
    x = (blend_ + x) / (1.0f + blend_ * x);
    x = 0.5f * x + 0.5f;
    return x;
}

std::span<PackFloat2 const> STFT::Process2(std::span<PackFloat2 const> main_frame,
                                           std::span<PackFloat2 const> side_frame, STFTWiener& wiener) {
    // 左声道：fft(mod*win)、fft(carry*win)、fft(carry) 三个谱输入
    for (int i = 0; i < fft_size_; ++i) {
        temp_main_[static_cast<size_t>(i)] =
            hann_window_[static_cast<size_t>(i)] * main_frame[static_cast<size_t>(i)][0];
        temp_side_[static_cast<size_t>(i)] =
            hann_window_[static_cast<size_t>(i)] * side_frame[static_cast<size_t>(i)][0];
        temp_carry_[static_cast<size_t>(i)] = side_frame[static_cast<size_t>(i)][0];
    }
    fft_.FFT({temp_main_.data(), static_cast<size_t>(fft_size_)}, real_main_, imag_main_);
    fft_.FFT({temp_side_.data(), static_cast<size_t>(fft_size_)}, real_side_, imag_side_);
    fft_.FFT({temp_carry_.data(), static_cast<size_t>(fft_size_)}, real_carry_, imag_carry_);
    wiener(*this, real_main_, imag_main_, real_side_, imag_side_, real_carry_, imag_carry_, 0);
    // glitch=false → IFFT(fft(carry*win))；true → IFFT(fft(carry))
    if (wiener.GetGlitch())
        fft_.IFFT({temp_main_.data(), static_cast<size_t>(fft_size_)}, real_carry_, imag_carry_);
    else
        fft_.IFFT({temp_main_.data(), static_cast<size_t>(fft_size_)}, real_side_, imag_side_);

    // 右声道
    for (int i = 0; i < fft_size_; ++i) {
        temp_main_[static_cast<size_t>(fft_size_ + i)] =
            hann_window_[static_cast<size_t>(i)] * main_frame[static_cast<size_t>(i)][1];
        temp_side_[static_cast<size_t>(fft_size_ + i)] =
            hann_window_[static_cast<size_t>(i)] * side_frame[static_cast<size_t>(i)][1];
        temp_carry_[static_cast<size_t>(fft_size_ + i)] = side_frame[static_cast<size_t>(i)][1];
    }
    fft_.FFT({temp_main_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_main_, imag_main_);
    fft_.FFT({temp_side_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_side_, imag_side_);
    fft_.FFT({temp_carry_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_carry_, imag_carry_);
    wiener(*this, real_main_, imag_main_, real_side_, imag_side_, real_carry_, imag_carry_, 1);
    if (wiener.GetGlitch())
        fft_.IFFT({temp_main_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_carry_, imag_carry_);
    else
        fft_.IFFT({temp_main_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_side_, imag_side_);

    // 打包输出帧
    for (int i = 0; i < fft_size_; ++i)
        output_frame_[static_cast<size_t>(i)] = {temp_main_[static_cast<size_t>(i)],
                                                 temp_main_[static_cast<size_t>(fft_size_ + i)]};
    return std::span<PackFloat2 const>{output_frame_};
}

} // namespace green_vocoder::dsp
